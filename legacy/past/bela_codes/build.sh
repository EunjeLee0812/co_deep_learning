#!/bin/sh

# 1. 현재 이 스크립트 파일이 있는 실제 경로를 찾아냅니다.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 2. '.' 대신 '$SCRIPT_DIR'을 넣어서 "이 폴더 자체"를 프로젝트 경로로 전달합니다.
"$SCRIPT_DIR/../scripts/build_project.sh" "$SCRIPT_DIR" --force