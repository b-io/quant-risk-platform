#!/usr/bin/env sh
# Import the project-local .env.sh into the current shell.
# Source this file manually, or let build.sh/test.sh/install.sh source it.

qrp_env_project_root="${QRP_PROJECT_ROOT:-$(pwd)}"
if [ -n "${QRP_LOCAL_ENV_FILE:-}" ]; then
    case "$QRP_LOCAL_ENV_FILE" in
        /*|[A-Za-z]:*) qrp_env_file="$QRP_LOCAL_ENV_FILE" ;;
        *) qrp_env_file="$qrp_env_project_root/$QRP_LOCAL_ENV_FILE" ;;
    esac
else
    qrp_env_file="$qrp_env_project_root/.env.sh"
fi

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

if [ -z "${VCPKG_TARGET_TRIPLET:-}" ] && [ "${QRP_ENV_QUIET:-0}" != "1" ]; then
    printf '%s\n' "Warning: VCPKG_TARGET_TRIPLET is not set. CMake presets may fail vcpkg manifest installation." >&2
fi

unset qrp_env_file
unset qrp_env_project_root
