==============================================================

Co-Deep Learning: Bela & PyBela 실시간 데이터 스트리밍 세팅 가이드

이 가이드는 팀원들이 Bela 보드와 Python 환경을 처음 세팅할 때
명령어를 순서대로 복사해서 실행할 수 있도록 작성되었습니다.

==============================================================

[1단계] 프로젝트 전체 코드 다운로드
작업을 진행할 폴더를 열고, 터미널에 아래 두 줄을 순서대로 입력하세요.

git clone https://github.com/EunjeLee0812/co_deep_learning.git
cd co_deep_learning

==============================================================

[2단계] 파이썬 가상환경 생성 및 실행
다른 프로젝트와의 충돌을 막기 위해 독립된 환경을 만듭니다. (맥/리눅스 기준)

python3 -m venv venv
source venv/bin/activate

(※ 윈도우 사용자는 source 대신 .\venv\Scripts\activate 를 입력하세요)

==============================================================

[3단계] 필수 라이브러리 설치
터미널 입력창 왼쪽에 (venv)가 떠 있는 상태에서 아래 명령어를 입력하세요.

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
Watcher 라이브러리가 포함된 코드를 벨라로 전송하고 백그라운드에서 실행합니다.

rsync -rvL ./bela/trill_streamer root@bela.local:Bela/projects
ssh root@bela.local "make -C /root/Bela stop PROJECT=trill_streamer run"

(터미널에 Running... 이라는 문구가 뜨면 성공입니다. 이 터미널 창은 끄지 말고 켜두세요)

==============================================================

[7단계] 주피터 노트북 실행
터미널 탭을 하나 새로 엽니다. 가상환경을 다시 켜준 뒤 주피터 노트북을 엽니다.

source venv/bin/activate
jupyter notebook

==============================================================

