#include <qrp/optimization/adapters/cvxpy_adapter.hpp>
#include <qrp/optimization/portfolio_optimization.hpp>
#include <qrp/optimization/models/risk_model.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace qrp::optimization {

namespace {

namespace fs = std::filesystem;

constexpr const char* kWorkerRelativePath = "python/qrp/optimization/cvxpy_worker.py";

std::optional<std::string> custom_param(const SolverConfig& config, const std::string& key) {
    const auto it = config.custom_params.find(key);
    if (it == config.custom_params.end() || it->second.empty()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::string> env_value(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string(value);
}

std::optional<fs::path> find_worker_from(fs::path start) {
    std::error_code ec;
    fs::path current = fs::absolute(start, ec);
    if (ec) {
        current = std::move(start);
        ec.clear();
    }

    if (fs::is_regular_file(current, ec)) {
        current = current.parent_path();
    }

    while (!current.empty()) {
        fs::path candidate = current / kWorkerRelativePath;
        if (fs::is_regular_file(candidate, ec)) {
            fs::path canonical = fs::weakly_canonical(candidate, ec);
            return ec ? candidate : canonical;
        }

        fs::path parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = std::move(parent);
    }

    return std::nullopt;
}

fs::path configured_path(const std::string& value) {
    fs::path path(value);
    if (path.is_relative()) {
        path = fs::absolute(path);
    }
    return path;
}

fs::path resolve_worker_path(const SolverConfig& config) {
    if (auto configured = custom_param(config, "cvxpy_worker_path")) {
        return configured_path(*configured);
    }
    if (auto configured = env_value("QRP_CVXPY_WORKER")) {
        return configured_path(*configured);
    }
    if (auto discovered = find_worker_from(fs::current_path())) {
        return *discovered;
    }
    if (auto discovered = find_worker_from(fs::path(__FILE__))) {
        return *discovered;
    }
    return {};
}

std::string resolve_python_executable(const SolverConfig& config) {
    if (auto configured = custom_param(config, "python_executable")) {
        return *configured;
    }
    if (auto configured = env_value("QRP_PYTHON_EXECUTABLE")) {
        return *configured;
    }
    return "python";
}

std::string unique_suffix() {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::ostringstream os;
    os << now;
    return os.str();
}

struct TempJsonFiles {
    fs::path request;
    fs::path response;

    ~TempJsonFiles() {
        std::error_code ec;
        if (!request.empty()) {
            fs::remove(request, ec);
        }
        ec.clear();
        if (!response.empty()) {
            fs::remove(response, ec);
        }
    }
};

OptimizationResult error_result(const std::string& message) {
    OptimizationResult result;
    result.status = SolverStatus::Error;
    result.message = message;
    return result;
}

#ifdef _WIN32
std::wstring widen_utf8(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        return std::wstring(value.begin(), value.end());
    }

    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), wide.data(), size);
    return wide;
}

std::wstring quote_windows_arg(const std::string& arg) {
    const std::wstring wide = widen_utf8(arg);
    if (wide.empty()) {
        return L"\"\"";
    }

    const bool needs_quotes = wide.find_first_of(L" \t\n\v\"") != std::wstring::npos;
    if (!needs_quotes) {
        return wide;
    }

    std::wstring quoted = L"\"";
    std::size_t backslashes = 0;
    for (const wchar_t ch : wide) {
        if (ch == L'\\') {
            ++backslashes;
        } else if (ch == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(ch);
            backslashes = 0;
        } else {
            quoted.append(backslashes, L'\\');
            backslashes = 0;
            quoted.push_back(ch);
        }
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

int run_process(const std::vector<std::string>& args) {
    if (args.empty()) {
        throw std::runtime_error("Cannot start an empty process command");
    }

    std::wstring command_line;
    for (const auto& arg : args) {
        if (!command_line.empty()) {
            command_line.push_back(L' ');
        }
        command_line += quote_windows_arg(arg);
    }

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};
    std::wstring mutable_command_line = command_line;

    if (!CreateProcessW(
            nullptr,
            mutable_command_line.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startup_info,
            &process_info)) {
        throw std::runtime_error("Failed to start CVXPY worker; CreateProcessW error " + std::to_string(GetLastError()));
    }

    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code = 1;
    GetExitCodeProcess(process_info.hProcess, &exit_code);
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    return static_cast<int>(exit_code);
}
#else
int run_process(const std::vector<std::string>& args) {
    if (args.empty()) {
        throw std::runtime_error("Cannot start an empty process command");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error(std::string("Failed to fork CVXPY worker: ") + std::strerror(errno));
    }

    if (pid == 0) {
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        throw std::runtime_error(std::string("Failed to wait for CVXPY worker: ") + std::strerror(errno));
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return -1;
}
#endif

} // namespace

CvxpyAdapter::CvxpyAdapter() {}

SolverCapabilities CvxpyAdapter::get_capabilities() const {
    SolverCapabilities caps;
    caps.supports_quadratic_objectives = true;
    caps.supports_linear_constraints = true;
    caps.supports_sparse_matrices = true;
    caps.supports_warm_start = true;
    return caps;
}

bool CvxpyAdapter::supports(const OptimizationProblem&) const {
    // For now, support most convex problems.
    // In a real implementation, we would check if objectives/constraints are supported.
    return true;
}

OptimizationResult CvxpyAdapter::solve(const OptimizationProblem& problem, const SolverConfig& config) {
    nlohmann::json request = serialize_problem(problem, config);

    fs::path worker_path = resolve_worker_path(config);
    std::error_code ec;
    if (worker_path.empty() || !fs::is_regular_file(worker_path, ec)) {
        return error_result("CVXPY worker not found. Set SolverConfig.custom_params[\"cvxpy_worker_path\"] or QRP_CVXPY_WORKER.");
    }

    const std::string suffix = unique_suffix();
    TempJsonFiles temp_files{
        fs::temp_directory_path() / ("qrp_opt_request_" + suffix + ".json"),
        fs::temp_directory_path() / ("qrp_opt_response_" + suffix + ".json")
    };
    
    {
        std::ofstream ofs(temp_files.request);
        if (!ofs.is_open()) {
            return error_result("Failed to create optimization request file: " + temp_files.request.string());
        }
        ofs << request.dump(4);
    }

    const std::string python_executable = resolve_python_executable(config);
    const std::vector<std::string> args = {
        python_executable,
        worker_path.string(),
        temp_files.request.string(),
        temp_files.response.string()
    };

    if (config.verbose) {
        std::cout << "Running CVXPY worker: " << python_executable << " " << worker_path.string() << std::endl;
    }
    
    int ret = 0;
    try {
        ret = run_process(args);
    } catch (const std::exception& e) {
        return error_result(e.what());
    }

    if (ret != 0) {
        return error_result("CVXPY worker failed with exit code " + std::to_string(ret));
    }
    
    // Read response
    std::ifstream ifs(temp_files.response);
    if (!ifs.is_open()) {
        return error_result("Failed to open response file from CVXPY worker: " + temp_files.response.string());
    }
    
    nlohmann::json response;
    try {
        ifs >> response;
    } catch (const std::exception& e) {
        return error_result("Failed to parse JSON response: " + std::string(e.what()));
    }
    
    return parse_result(response);
}

nlohmann::json CvxpyAdapter::serialize_problem(const OptimizationProblem& problem, const SolverConfig& config) {
    nlohmann::json j;
    j["name"] = problem.name;
    
    // Variables
    nlohmann::json vars = nlohmann::json::array();
    for (const auto& var : problem.variables) {
        nlohmann::json v;
        v["id"] = var.id;
        v["lb"] = var.lower_bound;
        v["ub"] = var.upper_bound;
        v["integer"] = var.is_integer;
        vars.push_back(v);
    }
    j["variables"] = vars;
    
    // Objectives
    nlohmann::json objs = nlohmann::json::array();
    for (const auto& obj : problem.objectives) {
        nlohmann::json o;
        o["type"] = obj->type();
        if (auto mv = std::dynamic_pointer_cast<MeanVarianceObjective>(obj)) {
            o["expected_returns"] = mv->expected_returns;
            o["risk_aversion"] = mv->risk_aversion;
        } else if (auto te = std::dynamic_pointer_cast<TrackingErrorObjective>(obj)) {
            o["benchmark_weights"] = te->benchmark_weights;
        } else if (auto mr = std::dynamic_pointer_cast<MaximizeReturnObjective>(obj)) {
            o["expected_returns"] = mr->expected_returns;
        }
        objs.push_back(o);
    }
    j["objectives"] = objs;
    
    // Constraints
    nlohmann::json cons = nlohmann::json::array();
    for (const auto& con : problem.constraints) {
        nlohmann::json c;
        c["type"] = con->type();
        if (auto le = std::dynamic_pointer_cast<LinearEqualityConstraint>(con)) {
            c["coefficients"] = le->coefficients;
            c["target"] = le->target_value;
        } else if (auto li = std::dynamic_pointer_cast<LinearInequalityConstraint>(con)) {
            c["coefficients"] = li->coefficients;
            if (li->lower_bound) c["lb"] = *li->lower_bound;
            if (li->upper_bound) c["ub"] = *li->upper_bound;
        } else if (auto to = std::dynamic_pointer_cast<TurnoverConstraint>(con)) {
            c["current_weights"] = to->current_weights;
            c["max_turnover"] = to->max_turnover;
        }
        cons.push_back(c);
    }
    j["constraints"] = cons;
    
    // Risk Model
    if (problem.risk_model) {
        nlohmann::json rm;
        rm["type"] = problem.risk_model->type();
        if (auto full = std::dynamic_pointer_cast<FullCovarianceModel>(problem.risk_model)) {
            rm["asset_ids"] = full->asset_ids;
            rm["covariance"] = full->covariance_matrix;
        } else if (auto factor = std::dynamic_pointer_cast<FactorRiskModel>(problem.risk_model)) {
            rm["asset_ids"] = factor->asset_ids;
            rm["factor_ids"] = factor->factor_ids;
            rm["exposures"] = factor->exposures;
            rm["factor_covariance"] = factor->factor_covariance;
            rm["specific_risk"] = factor->specific_risk;
        }
        j["risk_model"] = rm;
    }
    
    // Solver Config
    j["config"]["tolerance"] = config.tolerance;
    j["config"]["max_iterations"] = config.max_iterations;
    if (config.time_limit_sec) j["config"]["time_limit"] = *config.time_limit_sec;
    j["config"]["verbose"] = config.verbose;
    j["config"]["solver"] = "OSQP"; // Hardcoded for this backend
    
    return j;
}

OptimizationResult CvxpyAdapter::parse_result(const nlohmann::json& j) {
    OptimizationResult res;
    std::string status = j.at("status").get<std::string>();
    if (status == "optimal") res.status = SolverStatus::Solved;
    else if (status == "infeasible") res.status = SolverStatus::Infeasible;
    else if (status == "unbounded") res.status = SolverStatus::Unbounded;
    else res.status = SolverStatus::Error;
    
    if (j.contains("message")) res.message = j.at("message").get<std::string>();
    if (j.contains("objective_value")) res.objective_value = j.at("objective_value").get<double>();
    if (j.contains("optimal_values")) res.optimal_values = j.at("optimal_values").get<std::map<std::string, double>>();
    
    if (j.contains("solve_time_ms")) res.solve_time_ms = j.at("solve_time_ms").get<double>();
    
    return res;
}

} // namespace qrp::optimization
