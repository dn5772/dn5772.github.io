---
title: "seam 개발 일지 #2 - 설계를 일주일 만에 갈아엎었음"
description: "투명한 관리 레이어를 버리고 'AI 툴의 Docker'로. Sprint 1~7을 통째로 취소함"
slug: "seam-2"
date: 2026-03-24T09:00:00+09:00
draft: false
tags: ["seam", "AI", "설계", "피벗", "Docker", "YAML"]
categories: ["tool/seam"]
---

# seam 개발 일지 #2 - 설계를 일주일 만에 갈아엎었음

[#1]({{< ref "/post/seam/1-왜" >}})에서 6레이어 아키텍처랑 2티어 전략을 짜고, 다음 날 Phase 1 MVP 구현 계획(Sprint 1~7)까지 세웠다.

그리고 일주일 뒤에 그걸 전부 버렸다.

피벗 문서 맨 위에 이렇게 적었다.

> **기존 구현 계획**: `2026-03-17-seam-mvp-launch-design.md` — 본 문서로 **전면 대체**됨.
> 기존 Sprint 1–7은 취소되고 아래 로드맵으로 재수립.

## 뭐가 바뀌었나

```
[v0.4]  사용자 앱 → seam(투명 레이어) → AI 백엔드
[v0.5]  seam.yaml → seam run → 툴들 구동 + 메모리 공유 + 모델 라우팅
```

**seam = AI 실행 환경의 Docker.**

투명하게 끼어드는 레이어가 아니라, **환경을 정의하고 패키징해서 공유하는 런타임**으로 바꿨다.

## 왜 바꿨나

v0.4의 핵심 가치는 "설치만 하면 AI가 나를 기억함"이었다.
좋은데, **사용자가 그걸 말로 설명하기가 어렵다.**

"백그라운드에서 돌면서 메모리를 주입해주는 프록시"를 누구한테 설명해보면, 듣는 사람 표정이 애매해진다.
안 보이는 게 핵심인 제품은 자랑하기가 어렵다.

반면 이런 문제들은 말하자마자 통한다.

| 문제 | 해결 |
|---|---|
| 새 AI 툴 나올 때마다 세팅 이전이 두렵다 | `seam import` → `seam push` → 새 환경에서 `seam pull` |
| 팀 온보딩 시 AI 세팅 통일이 어렵다 | `seam pull company/backend-team && seam run` |
| 여러 AI 툴 간 맥락이 끊긴다 | 같은 환경 안의 툴들이 메모리 공유 |
| AI 툴 설정이 내 PC에만 있다 | 이미지로 패키징해서 레지스트리에 보존 |

**"Docker처럼 AI 환경을 통째로 공유한다"** 는 한 문장으로 끝난다.

## seam.yaml

Docker Compose의 `docker-compose.yaml` 에 해당하는 파일이다. AI 환경 전체를 선언한다.

```yaml
name: senior-backend-dev
version: 1.0.0
description: "TypeScript + Rust 백엔드 개발 환경"

tools:
  - id: code-agent
    type: native                  # seam이 서브프로세스로 실행하는 에이전트
    model: claude-sonnet
    prompt: ./prompts/backend.md  # 시스템 프롬프트 (빌드 시 이미지에 포함)
    skills: [git-commit, code-review, test-runner]

  - id: cursor
    type: external                # 기존 앱 래핑 — seam이 실행하진 않음
    app: cursor                   # env var 오버라이드로 Gateway에 연결

memory:
  shared: true      # 환경 내 모든 툴이 같은 메모리 파티션을 씀
  personal: true    # ~/.seam/memory/ 자동 마운트

models:
  default: claude-sonnet
  fast: llama3.2
  budget_monthly: 50      # USD 초과 시 fast 모델로 자동 폴백
  fallback_local: true

skills:                   # 환경 전역 스킬 (툴 레벨 스킬과 합집합)
  - git-commit
  - code-review
```

### native vs external

이 구분이 [#1]({{< ref "/post/seam/1-왜" >}})의 2티어를 **yaml 안으로 끌고 들어온 것**이다.

- **native**: seam 바이너리가 서브프로세스로 직접 실행한다. 스킬·라우팅 전부 적용된다
- **external**: Cursor 같은 기존 앱. seam이 실행하진 않고, 환경변수 오버라이드로 Gateway에 연결만 한다. 메모리 주입만 받는다

Tier 1/Tier 2가 "사용자가 어느 단계에 있나"였는데, 이제는 **"한 환경 안에서 툴마다 다름"** 이 됐다.
Cursor랑 native 에이전트를 같은 `seam.yaml` 에 적고 같이 띄울 수 있다.

## 이미지랑 레지스트리

Docker 비유를 끝까지 밀었다.

```bash
seam build              # seam.yaml → tar.gz 이미지 + .seam-lock
seam push me/my-setup   # 레지스트리에 올림
seam pull team/backend  # 남의 환경 받아옴
seam run                # 실행
```

레지스트리는 **GitHub 레포를 그대로 쓴다.** `seam-envs` 라는 레포에 Contents API로 파일을 올리고 내린다.
자체 서버를 안 만든 이유는 단순하다. 지금 단계에서 서버 운영은 과한다. 인증도 GitHub 토큰 하나면 끝난다.

## 버린 것들

피벗하면서 v0.4 설계의 상당 부분이 뒤로 밀렸다.

- **Trust Sandbox** (Layer 2). eBPF로 시스템 콜 감시하겠다는 계획. Phase 4로
- **에이전트 통신 버스**. 에이전트끼리 메시지 주고받기. 뒤로
- **Lazy Skill Router**. 스킬 지연 로딩. `seam.yaml` 에 필드만 남기고 구현은 뒤로
- **Semantic Memory**. 3레이어 메모리 중 하나. Episodic만 먼저

설계 문서는 안 지웠다. `docs/archive/` 에 `ai-os-v0_3.md`, `seam-v0_4.md` 로 넣어뒀다.
**나중에 돌아올 수도 있고, 왜 이렇게 됐는지 알려면 이전 버전이 있어야 한다.**

## 느낀 점

일주일 만에 갈아엎는 게 좀 허탈하긴 했는데, **코드를 한 줄도 안 쓴 상태에서 갈아엎은 게 다행**이었다.
Sprint 1~7을 반쯤 구현하고 나서 알았으면 훨씬 아팠을 것.

설계 문서를 먼저 쓰는 게 느려 보이는데, **버릴 때 싸게 버릴 수 있다.**

그리고 "안 보이는 게 핵심인 제품"은 만들기도 어렵고 설명하기도 어렵다는 걸 배웠다.
같은 기능이라도 `seam.yaml` 이라는 **눈에 보이는 파일 하나**를 중심에 두니까 설명이 통한다.

이제 진짜 코드 칠 차례다.
