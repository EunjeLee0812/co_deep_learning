#!/usr/bin/env bash
# Bela 보드로 코드 배포 예시 (IP/경로는 환경에 맞게 수정)
set -e
BELA_HOST="${BELA_HOST:-root@192.168.7.2}"
PROJECT_NAME="${PROJECT_NAME:-myinstrument}"
rsync -avz --exclude 'build/' bela/ "$BELA_HOST:Bela/projects/$PROJECT_NAME/"
echo "Deployed to $BELA_HOST:Bela/projects/$PROJECT_NAME/"
