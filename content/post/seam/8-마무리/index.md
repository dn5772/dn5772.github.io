---
title: "seam 개발 일지 #8 - 데몬, 설치 마법사, 그리고 멈춘 자리"
description: "3개월 계획을 사흘에 밀어넣고 멈췄다. 커밋 73개 회고"
slug: "seam-8"
date: 2026-03-27T14:11:00+09:00
draft: false
tags: ["seam", "Rust", "데몬", "LaunchAgent", "회고", "CLI"]
categories: ["tool/seam"]
---

# seam 개발 일지 #8 - 데몬, 설치 마법사, 그리고 멈춘 자리

[#7]({{< ref "/post/seam/7-hitl" >}})까지 오면 기능은 다 있다. 근데 **쓸 수 있는 물건은 아니었다.**

데몬을 어떻게 띄우는지, 재부팅하면 어떻게 되는지, 처음 설치한 사람이 뭘 해야 하는지가 비어 있었다.

## 00:43 — start / stop / restart

```
00:43  feat: seam start/stop/restart + seam service install/uninstall
```

`seam start` 가 데몬을 띄운다. 게이트웨이(`:11400`)랑 IPC 소켓(`daemon.sock`)이 같이 올라간다.

구조가 좀 특이한데, **바이너리가 자기 자신을 다시 실행한다.**

```
seam start  →  seam __gateway  (hidden command)
seam run    →  seam __daemon --env-yaml <path>
```

`__gateway`, `__daemon`, `__agent` 는 clap에서 `hide = true` 로 숨겨둔 커맨드다.
`seam --help` 에는 안 나오는데 내부적으로 자기를 부를 때 쓴다.

이렇게 한 이유는 **단일 바이너리를 지키려고**이다. 데몬 실행 파일을 따로 두면 설치할 게 두 개가 된다.
[#3]({{< ref "/post/seam/3-rust-cli" >}})에서 Rust를 고른 이유가 "`curl | sh` 한 줄"이었는데, 그걸 끝까지 밀려면 파일이 하나여야 한다.

### 재부팅해도 살아 있게

```bash
seam service install     # macOS LaunchAgent 등록
seam service uninstall
```

프록시라서 **항상 떠 있어야 의미가 있다.** 로그인할 때마다 `seam start` 를 쳐야 하면 아무도 안 쓴다.

macOS만 구현했고 Linux systemd는 안 했다. 지금도 안 되어 있다.

## 01:26 — 이름 하나 빠져서 안 뜨던 데몬

```
01:26  fix: __gateway 이름 누락 + config.toml 하위 호환 파싱
```

`__gateway` 를 clap enum에 정의는 했는데 **디스패치에서 빠져 있었다.** `seam start` 가 조용히 아무것도 안 한다.

같이 고친 게 설정 파일 하위 호환이다. [#6]({{< ref "/post/seam/6-router" >}})에서 `router_slider`, `ollama_base_url`, `classifier_model` 을 추가했는데,
**그 전에 만들어진 `config.toml` 에는 그 필드가 없다.** 파싱이 통째로 실패해서 설정이 날아간다.

없으면 기본값을 쓰게 바꿨다. 설정 파일은 한 번 만들어지면 사용자 기계에 계속 남으니까, **새 필드는 항상 옵셔널이어야 한다.**

## 01:52 — 설치하고 처음 켰을 때

```
01:52  feat: seam setup — 대화형 초기 설정 마법사
```

`seam start` 를 처음 치면 setup이 자동으로 뜬다. `first_run` 을 감지해서.

물어보는 건 이런 것들이다.

- API 키 (anthropic / openai / google / github) → **OS 키체인에 저장**
- [라우팅 슬라이더]({{< ref "/post/seam/6-router" >}}) 0~9
- Ollama 쓸 건지, 주소는 어딘지

`dialoguer` 로 만들었다. 설정 파일 포맷을 문서에서 찾아 읽게 하는 것보다 이게 낫다.
특히 **API 키를 어디다 둬야 하는지**는 물어보는 게 맞다. 안 그러면 다들 환경변수에 평문으로 둔다.

## 14:11 — 마지막 커밋

```
14:11  feat: setup wizard 언어 선택 + EN/KO 완전 이중 언어화
```

setup 첫 질문이 언어 선택이고, CLI 출력 전체를 영어/한국어로 갈랐다.

그리고 이게 **마지막 커밋이다.**

## 사흘, 커밋 73개

```
seam-cli   (Rust)    47 커밋
seam-code  (Python)  26 커밋
                     ─────
                     73 커밋   3/25 ~ 3/27
```

테스트는 81개.

| 모듈 | 테스트 |
|---|---|
| router | 12 |
| agents/code | 14 |
| gateway | 9 |
| cli | 8 |
| config / memory | 14 |
| ipc | 6 |
| 나머지 | 13 |
| **integration** | **5** |

론칭 계획 문서에 적어둔 일정은 이랬다.

```
M+0  Week 1-2   Task 1-4   scaffold, config, seam.yaml 파싱, 메모리
M+1  Week 3-4   Task 5-7   Gateway, Runner, CLI
M+2  Week 5-6   Task 8     Image build
M+2  Week 7-8   Task 9     GitHub registry
M+3  Week 9     Task 12    README, install.sh, 내부 테스트
```

**3개월 계획을 사흘에 했다.** Task 1~9가 [#3]({{< ref "/post/seam/3-rust-cli" >}})의 2시간 25분에 다 들어가 있다.

## 안 만든 것들

정직하게 적으면 이만큼 비어 있다.

- `seam journal` / `seam budget`. **stub만 있다.** [#1]({{< ref "/post/seam/1-왜" >}})의 킬 메시지가 "AI 활동 영수증"이었는데 그게 비어 있다
- `journal.rs` 는 작성됐는데 **프록시에서 호출을 안 함**
- `seam logs --follow`. 미구현
- `seam service`. macOS만. Linux systemd 없다
- **Trust Sandbox** (Layer 2). 통째로 없다
- **Lazy Skill Router** (Layer 3). `seam.yaml` 에 필드만 있고 구현 없다
- **에이전트 통신 버스**. 없다
- **Semantic Memory**. 없다. Episodic + Shared만
- `hub.seam.dev`. 레지스트리는 GitHub 레포로 대체

[#1]({{< ref "/post/seam/1-왜" >}})에서 적었던 6레이어 중 **실제로 만든 건 Layer 0, 1, 4 세 개**이다.

## 배운 것

### 설계를 먼저 쓰면 싸게 버릴 수 있음

[#2]({{< ref "/post/seam/2-피벗" >}})에서 일주일 만에 방향을 통째로 갈아엎었는데, **코드가 한 줄도 없는 상태**였다.
Sprint 1~7을 반쯤 짜고 알았으면 훨씬 아팠을 것.

문서 쓰는 게 느려 보여도, 버릴 때 손해가 작다.

### "안 보이는 제품"은 설명이 안 됨

v0.4는 "백그라운드에서 메모리를 주입하는 투명한 레이어"였다. 기능은 같은데 설명이 안 됐다.
`seam.yaml` 이라는 **파일 하나**를 중심에 두니까 한 문장으로 끝남. "Docker처럼 AI 환경을 공유함".

### 조용한 실패가 제일 위험함

- [#3]({{< ref "/post/seam/3-rust-cli" >}}). HTTP 4xx/5xx를 안 보고 지나간다
- [#4]({{< ref "/post/seam/4-seam-code" >}}). 종료 코드 0이 아닌데 성공 반환, 루프 상한에서 말없이 종료
- [#5]({{< ref "/post/seam/5-ipc" >}}). accept 루프가 통째로 죽음
- [#7]({{< ref "/post/seam/7-hitl" >}}). 라우터 응답이 비면 기본값으로 조용히 넘어간다

전부 다른 날 다른 언어에서 나왔는데 같은 모양이다. **에러를 안 내고 "그럴듯하게" 계속 가는 코드.**

### 빠른 게 마냥 좋진 않았음

3개월 계획을 사흘에 밀어넣은 건 기분이 좋았는데, 결과를 보면 **넓고 얕는다.**
레이어 6개 중 3개, 킬 기능인 journal/budget은 stub, Linux는 미지원.

돌이켜보면 **Phase 1만 제대로 끝냈으면 쓸 수 있는 물건이 됐을 것 같다.**
"설치만 하면 AI가 나를 기억한다 + 오늘 AI가 쓴 돈" 딱 여기까지. 그게 [#1]({{< ref "/post/seam/1-왜" >}})에서 정한 MVP 범위이기도 했다.

기능을 넓히는 속도가 붙으니까 **범위를 지키는 쪽이 오히려 어려웠다.**

## 지금

3/27 이후로 멈춰 있다.

`docs/superpowers/plans/2026-03-27-seam-phase-a-routing-memory.md` 가 다음 계획인데 실행을 안 했다.
CLAUDE.md의 "다음 단계"에도 `Task 10-11`, `hub.seam.dev` 가 그대로 남아 있다.

버린 프로젝트는 아니고, 멈춘 자리가 명확하다는 정도다.
돌아온다면 **새 기능이 아니라 journal이랑 budget부터** 할 것 같다. 그게 원래 제일 처음에 보여주기로 한 거였다.
