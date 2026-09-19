---
title: "seam 개발 일지 #7 - 에이전트한테 허락받게 만들었음"
description: "HITL 승인 게이트, 세션 영속, CLAUDE.md 자동 발견. seam-code 0.6"
slug: "seam-7"
date: 2026-03-27T00:19:00+09:00
draft: false
tags: ["seam", "에이전트", "HITL", "Python", "TUI", "UX"]
categories: ["tool/seam"]
---

# seam 개발 일지 #7 - 에이전트한테 허락받게 만들었음

[#4]({{< ref "/post/seam/4-seam-code" >}})에서 만든 seam-code는 **툴을 그냥 실행했다.**

`execute` 툴이 있고 에이전트가 셸 명령을 만들면, 그게 바로 돌아간다.
`write_file` 도 마찬가지. 에이전트가 판단하면 파일이 덮어써진다.

테스트할 땐 편한데, 실제로 쓰려니까 못 쓰겠다.

## HITL 게이트

Human-in-the-loop. 툴이 실행되기 **전에** 사람한테 물어본다.

```
22:34  feat: add HITLGate for tool execution approval
22:34  feat: add local context injection — CLAUDE.md / AGENTS.md auto-discovery
22:39  fix: use full path in context section headers to disambiguate same-named files
22:41  feat: add command registry — /model /sessions /compact /help /new
00:08  feat: add tool_panel (diff/shell output) and approval UI widgets
```

핵심은 **무엇을 보여주느냐**이다. "execute 툴을 실행할까요?" 라고만 물으면 아무 의미가 없다.
그래서 `tool_panel` 위젯을 만들어서 실제 내용을 띄운다.

- 파일 수정 → **diff**
- 셸 명령 → 실행할 **명령어 전문**

diff를 보여주는 게 결정적이었다. "이 파일을 고칠까요?"랑 "이 세 줄을 이렇게 바꿀까요?"는 완전히 다른 질문이다.
전자는 대충 예스를 누르게 되고, 후자는 실제로 읽게 된다.

## CLAUDE.md를 알아서 찾음

같은 시각 커밋인데 이게 체감이 제일 컸다.

프로젝트에 이미 `CLAUDE.md` 나 `AGENTS.md` 가 있으면 **그걸 자동으로 찾아서 컨텍스트에 넣는다.**

```
현재 디렉토리부터 위로 올라가며 CLAUDE.md / AGENTS.md 탐색
  → 찾은 것들을 컨텍스트 섹션으로 주입
```

새 도구를 쓸 때 제일 귀찮은 게 **프로젝트 규칙을 다시 설명하는 것**인데, 이미 파일로 적어둔 사람이 많다.
그걸 그냥 읽으면 된다.

22:39 커밋이 이걸 고친다. 여러 개를 찾았을 때 섹션 헤더가 전부 `CLAUDE.md` 라서 **어느 디렉토리 건지 구분이 안 됐다.**
전체 경로를 헤더에 넣게 바꾼다. 모노레포에서 루트랑 패키지에 각각 있으면 구분이 필요하다.

## 세션이 남음

```
00:05  feat: integrate session persistence, HITL gate, and local context into SeamAgent
00:09  feat: seam-code 0.6 — session persistence, HITL, context injection, command registry, tool panels
```

`/sessions` 로 이전 대화를 불러온다. `/compact` 로 긴 히스토리를 압축한다. `/new` 로 새로 시작.

커맨드 레지스트리도 같이 넣었다.

| 커맨드 | 하는 일 |
|---|---|
| `/model` | 모델 확인·변경 |
| `/sessions` | 이전 세션 목록 |
| `/compact` | 히스토리 압축 |
| `/new` | 새 세션 |
| `/help` | 도움말 |

## 조용한 폴백을 없앰

이날 커밋 중에 제일 마음에 드는 거다.

```
00:01  fix: raise on missing provider/model in route() response instead of silent fallback
```

[#6]({{< ref "/post/seam/6-router" >}})에서 만든 `route()` 가 프로바이더랑 모델을 돌려준다.
근데 응답에 그게 없으면 seam-code가 **기본값으로 조용히 넘어가고 있었다.**

이게 왜 나쁘냐면. 라우터가 고장 나도 **아무 일도 안 일어난 것처럼 보다.**
슬라이더를 0으로 맞춰놨는데 계속 비싼 모델이 불리고 있어도 모른다. 청구서를 봐야 앎.

에러를 내게 바꿨다. **틀린 채로 돌아가는 것보다 멈추는 게 낫다.**

[정담]({{< ref "/post/정담/7-보안" >}}) 때 "조용히 실패하는 버그가 제일 오래 산다"고 썼는데, 여기서도 같은 얘기다.

## IPC 클라이언트 확장

```
00:00  feat: add shared_memory_save, private_memory_save, route to IPC client
```

[#6]({{< ref "/post/seam/6-router" >}})에서 Rust 쪽에 만든 핸들러들을 파이썬에서 부를 수 있게 붙이다.
이걸로 **두 프로젝트의 0.6이 맞물린다.**

```
seam-cli  0.6  RouterState + SharedStore + IPC 핸들러
seam-code 0.6  IPC 클라이언트 + HITL + 세션 + 컨텍스트
```

## 마무리 두 개

```
00:14  fix: resolve unused import warnings in TUI modules
00:19  fix: address final code review findings
```

## 느낀 점

**에이전트한테 권한을 주는 것보다 뺏는 게 어려웠다.**

툴을 그냥 실행하게 두는 게 코드는 훨씬 단순하다. 승인 게이트를 넣으면 TUI랑 에이전트 루프가 엮이고, 비동기 흐름이 복잡해진다.
근데 그걸 안 하면 **무서워서 못 쓴다.** 실제로 쓸 수 있는 물건이 되려면 필요한 비용이었다.

`CLAUDE.md` 자동 발견은 코드 몇 줄인데 효과가 제일 컸다.
**새 도구가 기존 관습을 존중하면 진입 비용이 확 내려간다.** 내 형식을 새로 만들지 않은 게 잘한 것 같다.
