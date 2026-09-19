---
title: "seam 개발 일지 #3 - Rust CLI를 2시간 25분에 밀어넣었음"
description: "스캐폴드부터 GitHub 레지스트리까지 커밋 18개. 3개월 계획이었는데"
slug: "seam-3"
date: 2026-03-25T17:37:00+09:00
draft: false
tags: ["seam", "Rust", "CLI", "axum", "clap", "keyring"]
categories: ["tool/seam"]
---

# seam 개발 일지 #3 - Rust CLI를 2시간 25분에 밀어넣었음

[#2]({{< ref "/post/seam/2-피벗" >}})에서 `seam.yaml` 로 방향을 잡고, 바로 Rust로 짜기 시작했음.

커밋 시각을 보면 이럼.

```
15:12  feat: project scaffold with clap CLI, lib.rs, stub modules
...
17:37  fix: propagate proxy body read errors as 502, wire journal writes
```

**2시간 25분에 커밋 18개.** 론칭 계획에는 이 범위가 `M+0 ~ M+3`, 그러니까 3개월로 잡혀 있었음.

## 🦀 왜 Rust인가

단일 바이너리로 떨어지는 게 제일 컸음.

```bash
curl -fsSL install.seam.dev | sh
```

이 한 줄로 끝나야 하는데, Python이면 런타임이랑 의존성이 따라옴. Node면 더 심함.
**"AI 환경 관리 도구를 쓰려고 환경을 관리해야 하는"** 상황이 되면 안 됐음.

게다가 데몬으로 계속 떠 있어야 하고, 프록시라서 모든 AI 요청이 여길 통과함. 메모리 사용량이랑 지연이 그대로 체감됨.

## 🧱 15:12~15:33 — 뼈대

```
15:12  project scaffold with clap CLI, lib.rs, stub modules
15:18  fix: normalize Pull dispatch path, remove unreachable wildcard arm
15:26  feat: config init, ~/.seam/ dir structure, keyring key management
15:28  feat: seam.yaml parsing with SeamConfig, ToolType, skills union
15:30  feat: episodic memory with env_id partitioning and context injection
15:32  feat: HTTP gateway with memory injection, proxy to Anthropic/OpenAI, journal
15:33  fix: use &Path instead of &PathBuf in journal::append
```

**20분 만에 게이트웨이까지 갔음.**

### API 키는 OS 키체인에

`.env` 나 config 파일에 평문으로 두는 걸 처음부터 안 하기로 했음.
`keyring` 크레이트를 쓰면 macOS Keychain / Linux Secret Service에 그대로 들어감.

```bash
seam key add anthropic sk-ant-...
seam key list
```

설정 파일(`~/.seam/config.toml`)에는 키가 안 들어감. 포트, 기본 모델 같은 것만 들어감.

### 스토리지는 SQL 없이 JSONL

문서 버전 이력에 이렇게 적혀 있음.

> v0.6: SQL 제거 → 파일 기반 스토리지 확정

메모리를 처음엔 SQLite로 잡았다가 JSONL(append-only)로 바꿨음.

```
~/.seam/memory/episodic.jsonl    # env_id로 파티셔닝, append-only
~/.seam/journal/YYYY-MM-DD.jsonl # 날짜별 활동 기록
```

append-only라 쓰기가 단순하고, 사람이 `tail -f` 로 그냥 볼 수 있음.
[정담]({{< ref "/post/정담/4-저장소" >}}) 때랑 비슷한 결론인데, 데이터가 작고 동시 쓰기가 없으면 DB가 필요 없음.

## ⚙️ 15:35~15:43 — 프로세스 관리

```
15:35  feat: runner — process manager, external tool env files, PID management
15:40  feat: add watch_with_restart to runner process manager
15:43  fix: replace phantom re-spawn with correct Crashed status
```

`seam run` 이 `seam.yaml` 의 native 툴들을 서브프로세스로 띄움. 그럼 **누가 살아 있는지** 알아야 함.

```
~/.seam/run/<name>.pid
```

PID 파일 쓰고, 살아 있는지는 `libc::kill(pid, 0)` 으로 확인함. 시그널 0은 실제로 안 보내고 **보낼 수 있는지만** 검사함. macOS/Linux 공통으로 됨.

15:43 커밋이 재밌는데, `watch_with_restart` 가 크래시를 감지하고 **자기가 다시 띄우려고** 했었음.
그러면 재시작 책임이 두 군데(감시자 + 코디네이터)로 갈라져서 유령 프로세스가 생김. `Crashed` 상태로 끊고 나가게 바꿨음.

## 🖥️ 15:43~17:17 — CLI 서브커맨드

```
16:58  feat: CLI — run/stop/ps/logs/status subcommands with unit tests
17:17  fix: CLI quality — check kill return, fix unwrap on path, PID parse safety,
              follow warning, status liveness check
```

17:17 커밋이 자잘한 것만 모아놨는데 전부 같은 종류임.

- `kill()` 반환값을 안 보고 성공으로 친 것
- 경로 변환에서 `unwrap()` 쓴 것
- PID 파일 내용이 숫자가 아닐 때 터지는 것
- `--follow` 가 미구현인데 아무 말 없이 조용한 것
- `status` 가 PID 파일만 보고 "실행 중"이라고 한 것 (프로세스는 죽었는데)

**전부 "실패 경로를 안 본 것"임.** 잘 돌아갈 때만 생각하고 짠 코드.

## 📦 17:19~17:28 — 이미지랑 레지스트리

```
17:19  feat: image build — seam.yaml → tar.gz with .seam-lock generation
17:23  feat: GitHub registry — pull seam.yaml from seam-envs repo, push with auto-create
17:28  fix: add HTTP error checking in registry pull/push, check repo creation response
```

`seam build` 가 `seam.yaml` 이랑 프롬프트 파일들을 `tar.gz` 로 묶고 `.seam-lock` 을 만듦.
Docker 이미지 흉내인데 레이어는 없음. 그냥 압축 파일 하나임.

레지스트리는 GitHub Contents API를 씀. 레포가 없으면 만들어줌.

17:28 커밋이 중요한데, **HTTP 응답 상태를 아무도 안 보고 있었음.**

```rust
// 전 — 404가 와도 그냥 지나감
let resp = client.get(url).send().await?;

// 후
let resp = client.get(url).send().await?.error_for_status()?;
```

`reqwest` 는 4xx/5xx를 에러로 안 봄. 네트워크가 성공했으면 성공임.
`.error_for_status()?` 를 안 붙이면 **404 응답을 파싱하려다 이상한 데서 터짐.** 레포 생성 응답도 마찬가지였음.

## 🔌 17:34~17:37 — 마무리

```
17:34  docs: README and install script
17:37  fix: propagate proxy body read errors as 502, wire journal writes on each request
```

프록시가 바디를 읽다가 실패하면 그걸 삼키고 있었음. 502로 올려보내게 바꿈.

## 🎯 느낀 점

**18개 중 7개가 `fix:` 였고, 전부 직전 `feat:` 을 고치는 거였음.**

```
15:40 feat: watch_with_restart      → 15:43 fix: 유령 재실행 제거
17:19 feat: image build             → 17:21 fix: SeamLock pub, 경로, mtime
17:23 feat: GitHub registry         → 17:28 fix: HTTP 에러 체크
```

빠르게 밀고 바로 되돌아와서 고치는 식이었음. 기능 하나 넣고 다시 읽어보면 실패 경로가 비어 있음.
**"돌아가게 만들기"랑 "안 터지게 만들기"가 다른 작업**이라는 게 커밋 로그에 그대로 남았음.

그리고 3개월 계획을 2시간 반에 끝낸 건 기분은 좋은데, 이게 마냥 좋은 일인지는 [마지막 글]({{< ref "/post/seam/8-마무리" >}})에서 다시 생각해봄.

#seam #Rust #CLI #axum #clap
