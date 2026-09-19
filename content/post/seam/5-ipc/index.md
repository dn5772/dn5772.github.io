---
title: "seam 개발 일지 #5 - Rust랑 Python을 유닉스 소켓으로 이었음"
description: "HTTP 대신 Unix domain socket. JSON Lines 프로토콜이랑 0o600 퍼미션"
slug: "seam-5"
date: 2026-03-26T16:34:00+09:00
draft: false
tags: ["seam", "IPC", "UnixSocket", "Rust", "Python", "보안"]
categories: ["tool/seam"]
---

# seam 개발 일지 #5 - Rust랑 Python을 유닉스 소켓으로 이었음

[#4]({{< ref "/post/seam/4-seam-code" >}})에서 Python 에이전트를 만들었는데, 얘가 Rust 데몬한테 말을 걸어야 함.

- 메모리 저장해줘
- 이 요청 어느 모델로 보낼지 정해줘
- 이 대화 처리해줘

게이트웨이가 이미 HTTP로 `:11400` 에 떠 있으니까 거기다 붙이면 되는데, **일부러 안 했음.**

## 🔌 왜 HTTP가 아니라 유닉스 소켓인가

게이트웨이 포트는 **Tier 1용**임. Cursor 같은 외부 앱이 환경변수 오버라이드로 들어오는 입구.
여기는 OpenAI/Anthropic 호환 형식을 지켜야 하고, 네트워크에 열려 있음.

근데 seam-code가 쓰는 건 성격이 다름.

- 같은 기계 안에서만 씀
- 형식을 남의 API에 맞출 이유가 없음
- **다른 사용자가 접근하면 안 됨**

포트로 열면 그 기계의 누구나 붙을 수 있음. 유닉스 소켓은 파일이라 **파일 퍼미션으로 막을 수 있음.**

```
~/.seam/daemon.sock   (0o600 — 소유자만)
```

`0o600` 을 CLAUDE.md 주의사항에 따로 적어뒀음. 이게 빠지면 메모리랑 API 키를 쓰는 경로가 통째로 열림.

## 📝 JSON Lines

프로토콜은 제일 단순한 걸로 했음. **한 줄에 JSON 하나.**

```
{"type":"memory_save","content":"RS256 선택함","env_id":"backend"}\n
{"type":"route","text":"이 함수 리팩터링해줘"}\n
{"type":"chat","messages":[...]}\n
```

gRPC나 MessagePack도 생각했는데 안 갔음.

- **스키마 정의 파일이 없음.** 양쪽(Rust/Python)에 코드 생성기를 붙일 필요가 없음
- **`nc -U ~/.seam/daemon.sock` 으로 손으로 찔러볼 수 있음.** 디버깅할 때 이게 큼
- 줄바꿈이 메시지 경계라 파싱이 그냥 `read_line()` 임

핸들러는 네 종류임.

| 타입 | 하는 일 |
|---|---|
| `chat` | 메모리 주입 + LLM 호출 |
| `memory_save` | private(episodic) 메모리 저장 |
| `shared_memory_save` / `shared_memory_get` | 공유 메모리 |
| `route` | 이 요청 어느 모델로 갈지만 알려줌 |

## 🦀 10:28~10:59 — Rust 쪽 서버

```
10:28  feat: add ipc module scaffold
10:29  feat: implement Unix socket IPC server accept loop
10:31  feat: implement memory_save IPC handler
10:33  feat: implement chat IPC handler with memory injection and Anthropic API
10:36  feat: start IPC server alongside HTTP gateway in daemon
10:59  fix: socket permissions, accept loop resilience, shared http client, configurable model
```

31분인데 마지막 커밋 하나가 23분 걸렸음. 거기 들어간 게 전부 **"이게 오래 떠 있을 때"** 문제임.

**1. 소켓 퍼미션.** 만들 때 퍼미션을 안 정하면 umask를 따라감. 명시적으로 `0o600` 으로 박음.

**2. accept 루프 복원력.** 연결 하나 받다가 에러 나면 **루프가 통째로 죽고 있었음.**
클라이언트 하나가 이상하게 끊었을 뿐인데 데몬의 IPC가 영영 멈춤. 에러를 로그만 찍고 루프는 계속 돌게 바꿈.

**3. HTTP 클라이언트 공유.** 요청마다 `reqwest::Client` 를 새로 만들고 있었음.
커넥션 풀이랑 TLS 세션이 매번 버려짐. 하나 만들어서 돌려쓰게 함.

**4. 모델 하드코딩.** 설정에서 읽게 바꿈.

## 🐍 파이썬 쪽 클라이언트

`seam_code/ipc.py` 가 소켓에 붙어서 한 줄 쓰고 한 줄 읽음. 데몬이 안 떠 있으면 먼저 알려줌.

```
09:53  feat: add config loader and daemon startup check
```

**에이전트를 띄우기 전에 데몬 체크를 먼저 함.** 안 그러면 사용자가 요청을 다 친 다음에 "연결 실패"를 봄.

## 🔗 15:48~16:34 — 두 개를 CLI로 묶기

```
15:48  feat: add seam install command
15:55  fix: assert Err in test_install_fake_app_fails_gracefully
16:16  fix: add pip -- separator to prevent option injection, mark pip test as ignored
16:22  feat: add seam code command — thin wrapper for seam-code agent
16:25  fix: make check_daemon testable, add assertions to code command tests
16:34  fix: prefer pip3 over pip for install command
```

```bash
seam install seam-code   # pip으로 설치
seam code "요청"          # 실행
```

`seam code` 는 **얇은 래퍼**임. 데몬 살아 있는지 보고 seam-code를 띄우는 게 전부.
Rust에서 에이전트 로직을 다시 만들지 않았음.

### pip에 `--` 를 붙인 이유

16:16 커밋이 짧은데 중요함.

```rust
// 전
Command::new("pip3").args(["install", app_name])

// 후
Command::new("pip3").args(["install", "--", app_name])
```

`seam install <app>` 의 `<app>` 이 사용자 입력임. 여기에 `--upgrade` 나 `--index-url ...` 같은 걸 넣으면 **pip 옵션으로 해석됨.**
`--` 는 "여기부터는 옵션이 아니라 인자"라는 표시라서 그 경로가 막힘.

임의 명령 실행까지는 아닌데(셸을 안 거침), 설치 소스를 바꿔치기하는 정도는 됨.

## 🎯 느낀 점

**입구를 두 개로 나눈 게 맞았음.**

```
:11400  HTTP    ← 외부 앱 (Tier 1). 남의 API 형식을 지켜야 함
daemon.sock     ← 내 앱 (Tier 2). 내 맘대로, 대신 퍼미션으로 잠금
```

하나로 합쳤으면 seam-code용 엔드포인트가 네트워크에 노출됐을 것. 퍼미션으로 막을 수 있는 걸 인증 로직으로 막게 됐을 거고.

그리고 10:59 커밋에 몰린 네 가지가 전부 **"짧게 돌 땐 안 보이는 문제"** 였음.
데몬처럼 오래 떠 있는 프로그램은 accept 루프 하나 죽는 게 치명적인데, 테스트는 보통 한 번 붙었다 끊는 걸로 끝나서 안 잡힘.

#seam #IPC #UnixSocket #Rust #Python
