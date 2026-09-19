---
title: "seam 개발 일지 #4 - 17분 만에 만든 코딩 에이전트"
description: "seam 위에서 도는 첫 앱. deepagents를 포크해서 Python 에이전트를 붙임"
slug: "seam-4"
date: 2026-03-26T10:06:00+09:00
draft: false
tags: ["seam", "Python", "에이전트", "Textual", "TUI", "LLM"]
categories: ["tool/seam"]
---

# seam 개발 일지 #4 - 17분 만에 만든 코딩 에이전트

[#3]({{< ref "/post/seam/3-rust-cli" >}})에서 런타임은 만들었는데, **그 위에서 돌 앱이 없었음.**

`seam.yaml` 에 `type: native` 툴을 적을 수 있게 해놨지만 정작 띄울 게 없음.
플랫폼만 있고 앱이 없는 상태라 아무것도 증명이 안 됨.

그래서 첫 공식 앱을 만들었음. **seam-code** — 터미널 코딩 에이전트.

## 🐍 왜 Python인가

[#3]({{< ref "/post/seam/3-rust-cli" >}})에서 Rust를 고른 이유가 "단일 바이너리"였는데, 여기선 정반대로 갔음.

에이전트 쪽은 **생태계가 전부 Python**임. LangChain, 각종 LLM SDK, TUI 라이브러리까지.
deepagents(LangChain, MIT)를 포크해서 시작했는데, 이걸 Rust로 다시 짤 이유가 없었음.

그래서 **듀얼 프로젝트**가 됨.

```
seam-cli   (Rust)    런타임 · 게이트웨이 · 데몬 · 라우터
seam-code  (Python)  코딩 에이전트 · TUI
```

역할이 갈림. **seam-cli가 모델 라우팅이랑 메모리를 맡고, seam-code는 툴 실행이랑 UX에 집중함.**

## ⏱️ 09:49~10:06, 커밋 13개

```
09:49  feat: scaffold seam-code project
09:53  feat: add config loader and daemon startup check
09:57  feat: implement seam daemon IPC client
09:59  feat: implement local tool set (files, shell, search, memory_save)
10:01  fix: execute returns error key on non-zero exit, grep_files returns sorted list
10:03  feat: implement IPC-based agent loop with tool dispatch
10:04  fix: append fallback message to history on MAX_TOOL_LOOPS exit
10:06  feat: add headless execution mode
10:06  feat: add Textual TUI with chat interface
10:06  feat: add seam.yaml image definition and default system prompt
```

**17분임.** 스캐폴드부터 TUI까지.

## 🔧 툴 8개

에이전트가 쓸 수 있는 도구를 로컬에 뒀음.

| 툴 | 하는 일 |
|---|---|
| `read_file` / `write_file` / `edit_file` | 파일 읽기·쓰기·수정 |
| `execute` | 셸 명령 |
| `glob` / `grep` | 파일 찾기 |
| `todo` | 작업 목록 |
| `memory_save` | seam 메모리에 저장 |

`memory_save` 만 IPC로 seam-cli에 넘어감. 나머지는 전부 로컬에서 처리.
**메모리만 공유하고 파일 작업은 각자 하는 구조**임.

10:01 커밋이 툴 두 개를 고치는데 둘 다 같은 종류임.

- `execute` 가 **종료 코드가 0이 아닌데 성공으로 반환**하고 있었음. 에이전트가 "명령이 잘 됐구나" 하고 다음 단계로 감
- `grep_files` 가 **순서가 매번 달랐음.** 파일시스템 순회 순서 그대로라서 같은 요청에 다른 결과가 나옴

두 번째는 특히 에이전트한테 나쁨. **같은 상황에서 다르게 행동하면 디버깅이 불가능해짐.** 정렬해서 반환하게 바꿈.

## 🔁 에이전트 루프

핵심은 이거 하나임.

```
사용자 요청
  → LLM 호출 (툴 목록 같이 전달)
  → LLM이 툴 호출을 요청
  → 툴 실행
  → 결과를 히스토리에 넣고 다시 LLM 호출
  → 툴 호출이 없으면 종료
```

무한 루프가 될 수 있어서 `MAX_TOOL_LOOPS` 로 상한을 뒀음.

10:04 커밋이 그 상한에 걸렸을 때를 고침. 원래는 **그냥 조용히 빠져나갔음.**
사용자 입장에선 에이전트가 아무 말 없이 멈춘 것처럼 보임. 히스토리에 "한도에 걸려서 멈췄다"는 메시지를 넣고 나가게 바꿈.

에이전트가 실패할 때 **왜 실패했는지 말하게 하는 건** 생각보다 중요한 것 같음.

## 🖥️ 모드 두 개

**headless** — 단발 실행.

```bash
seam code "이 코드베이스 설명해줘"
```

**TUI** — Textual로 만든 대화형 화면.

```bash
seam code
```

headless를 먼저 만들었음. 스크립트에서 부르기 쉽고, **TUI 없이도 에이전트 루프를 테스트할 수 있어서**임.
TUI를 먼저 만들었으면 화면이랑 로직이 엉켜서 테스트가 어려웠을 것.

## 🎯 seam-code가 다른 에이전트랑 다른 점

문서에 비교표를 적어놨는데, 결국 전부 **seam 위에 있다**는 데서 나옴.

| 기능 | 보통의 에이전트 | seam-code |
|---|---|---|
| 세션 간 메모리 | 로컬 SQLite | seam episodic memory — 다음 세션에도 맥락 유지 |
| 모델 라우팅 | 수동 플래그 | seam Gateway 자동 라우팅 |
| 팀 세팅 공유 | ❌ | `seam push me/code-setup` |
| 팀 온보딩 | ❌ | `seam pull team/code-standard && seam run` |

시니어가 쓰던 프롬프트랑 스킬 세팅을 새 팀원이 **한 줄로 받아 쓰는 것** — 이게 [#2]({{< ref "/post/seam/2-피벗" >}})에서 피벗한 이유 그 자체임.
플랫폼 혼자서는 못 보여주는 걸 앱이 하나 생기니까 보여줄 수 있게 됨.

## 🎯 느낀 점

Rust로 다 통일하고 싶은 마음이 있었는데, **언어를 갈라놓은 게 맞았음.**
런타임은 단일 바이너리여야 하고 에이전트는 생태계를 타야 함. 요구사항이 정반대라 억지로 한쪽에 맞췄으면 둘 다 나빠졌을 것.

대신 **두 프로세스가 대화할 방법**이 필요해짐. 그게 다음 글임.

#seam #Python #에이전트 #Textual #TUI
