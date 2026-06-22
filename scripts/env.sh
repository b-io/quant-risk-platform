#!/usr/bin/env sh
# Import the project-local .env.sh into the current shell.
# Source this file manually, or let build.sh/test.sh/install.sh source it.

qrp_env_project_root="${QRP_PROJECT_ROOT:-$(pwd)}"
qrp_env_file="${QRP_LOCAL_ENV_FILE:-$qrp_env_project_root/.env.sh}"

if [ -f "$qrp_env_file" ]; then
    QRP_ENV_FILE="${QRP_ENV_FILE:-$qrp_env_file}"
    export QRP_ENV_FILE
    # shellcheck disable=SC1090
    . "$qrp_env_file"
elif [ "${QRP_ENV_QUIET:-0}" != "1" ]; then
    printf '%s\n' "No environment file found at $qrp_env_file; using current process environment." >&2
fi

if [ "${QRP_ENV_QUIET:-0}" != "1" ]; then
    printf '%s\n' "QRP env profile      : ${QRP_ENV_PROFILE:-}"
    printf '%s\n' "QRP env file         : ${QRP_ENV_FILE:-}"
    printf '%s\n' "DEV_TOOLS_ROOT       : ${DEV_TOOLS_ROOT:-}"
    printf '%s\n' "VCPKG_ROOT           : ${VCPKG_ROOT:-}"
    printf '%s\n' "VCPKG_TARGET_TRIPLET : ${VCPKG_TARGET_TRIPLET:-}"
fi

if [ -z "${VCPKG_ROOT:-}" ] && [ "${QRP_ENV_QUIET:-0}" != "1" ]; then
    printf '%s\n' "Warning: VCPKG_ROOT is not set. CMake presets may not find vcpkg." >&2
fi

unset qrp_env_file
unset qrp_env_project_root
