# Tactor
Tactor is a musical touch interface with AI to give the user the experience of various musical techniques. We have used Bela platform to get low latency.

## Demo

## Installation and set up

### Installing pybela
#### 1. Python 3.12 이하
pip을 통해 설치 가능
```
pip install pybela
```
#### 2. Python 3.13 이상
uv 등을 통해 Python 3.12 이하의 가상 환경을 구축한 후 해당 환경에서 pybela 설치 및 코드 실행
```
uv venv --python 3.12
.venv\Scripts\activate
uv pip install pybela
```


co_deep_learning/
├── README.md
├── requirements.txt
│
├── bela/
│   ├── render.cpp
│   ├── Watcher.h
│   └── Watcher.cpp
│
├── notebooks/
└── src/