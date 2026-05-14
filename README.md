==============================================================

Co-Deep Learning: Bela & PyBela 실시간 데이터 스트리밍 세팅 가이드

이 가이드는 팀원들이 Bela 보드와 Python 환경을 처음 세팅할 때
명령어를 순서대로 복사해서 실행할 수 있도록 작성되었습니다.

==============================================================

[Docker로 환경 세팅하기] ← 추천: 모든 OS에서 동일한 환경 보장

Docker Desktop이 설치되어 있다면 아래 3줄이면 끝납니다.

  git clone https://github.com/EunjeLee0812/co_deep_learning.git
  cd co_deep_learning
  docker compose up --build

브라우저에서 http://localhost:8888 을 열면 Jupyter Notebook이 실행됩니다.
(비밀번호 없이 바로 접속됩니다.)

코드를 수정하면 컨테이너를 재시작하지 않아도 바로 반영됩니다.

⚠️ Bela 보드 USB 연결이 필요한 경우 (라이브 스트리밍)
  - Linux: docker-compose.yml 내 network_mode: host 주석을 해제하세요.
  - Windows/Mac: docker-compose.yml 파일 하단 주석을 참고하거나,
    호스트 터미널에서 직접 pybela를 실행하세요.

컨테이너 종료:
  docker compose down

==============================================================

[venv로 직접 세팅하기] ← Docker 없이 로컬에 설치할 경우

==============================================================

[1단계] 프로젝트 전체 코드 다운로드
작업을 진행할 폴더를 열고, 터미널에 아래 두 줄을 순서대로 입력하세요.

git clone https://github.com/EunjeLee0812/co_deep_learning.git
cd co_deep_learning

==============================================================

[2단계] 파이썬 가상환경 생성 및 실행 (⚠️ 중요: Python 3.10 또는 3.11 권장)
pybela 라이브러리는 너무 최신 버전의 파이썬(3.12 이상)에서는 에러가 발생합니다.
반드시 Python 3.10 또는 3.11 버전으로 가상환경을 만들어야 합니다.

안정적인 파이썬 버전(예: 3.10)으로 가상환경(venv)을 생성합니다.
python3.10 -m venv venv

가상환경을 켭니다. (맥/리눅스 기준)
source venv/bin/activate

(※ 윈도우 사용자는 source 대신 .\venv\Scripts\activate 를 입력하세요)

==============================================================

[3단계] 필수 라이브러리 설치
터미널 입력창 왼쪽에 (venv)가 떠 있는 상태에서 아래 명령어를 입력하세요.
source venv/bin/activate
pip install --upgrade pip
pip install pybela notebook pandas matplotlib

==============================================================

[4단계] Bela 보드 시간 동기화

--> 걍 Bela IDE 연결하면 됨. 아래꺼 무시.

Bela 보드를 컴퓨터에 USB로 연결합니다. 보드의 파란색 LED가 깜빡이기 시작하면
아래 명령어를 입력하여 컴퓨터의 시계와 Bela의 시계를 맞춥니다.
(비밀번호를 묻는다면 그냥 엔터를 치세요)

ssh root@bela.local "date -s "date '+%Y%m%d %T %z'""

==============================================================

[5단계] Bela 코어 업데이트
Bela 펌웨어를 최신 dev 버전으로 업데이트합니다.

--> 이미 세팅된 기기라면 건너뛰어도 됩니다

cd bela
git remote add board root@bela.local:Bela/
git push -f board dev:tmp
ssh root@bela.local "cd Bela && git checkout tmp && make -f Makefile.libraries cleanall && make coreclean"
cd ..

==============================================================

[6단계] 스트리밍 코드 벨라에 넣고 실행하기
깃허브에서 다운받은 내 노트북의 최신 코드를 벨라 보드로 전송하여 실행합니다.
(벨라 IDE 웹 화면을 켤 필요 없이 터미널에서 모두 끝납니다.)

내 노트북 코드를 벨라 보드로 복사 (rsync 명령어):
rsync -rvL ./bela/trill_streamer root@bela.local:Bela/projects

벨라 실행 (기존 프로세스 중지 및 새 코드 켜기):
ssh root@bela.local "make -C /root/Bela stop PROJECT=trill_streamer run"

(터미널에 'Running...' 이라는 문구가 뜨면 데이터 쏠 준비가 완료된 것입니다.
파이썬에서 데이터를 받는 동안 이 터미널 창은 절대 끄지 말고 켜두세요!)

==============================================================

[7단계] 주피터 노트북 실행
터미널 탭을 하나 새로 엽니다(맥북 : cmd + tab). 가상환경을 다시 켜준 뒤 주피터 노트북을 엽니다.

source venv/bin/activate
jupyter notebook

==============================================================

