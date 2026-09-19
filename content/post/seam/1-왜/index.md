---
title: "seam 개발 일지 #1 - AI 도구들이 각자의 섬이라서"
description: "Cursor에서 하던 작업을 Claude Desktop이 모름. 그 사이에 뭘 하나 끼워넣고 싶었음"
slug: "seam-1"
date: 2026-03-16T20:00:00+09:00
draft: false
tags: ["seam", "AI", "설계", "아키텍처", "Rust", "LLM"]
categories: ["tool/seam"]
---

# seam 개발 일지 #1 - AI 도구들이 각자의 섬이라서

AI 도구를 여러 개 쓰다 보면 계속 걸리는 게 있음.

Cursor에서 인증 API를 만들다가 Claude Desktop을 열면, **맥락을 처음부터 다시 설명해야 함.**
어제 AI랑 한 작업을 오늘 AI는 모름. 같은 컴퓨터, 같은 사람인데.

이걸 해결하는 걸 만들어보기로 했음. 오늘은 코드 한 줄도 안 쓰고 설계 문서만 10개 썼음.

## 🏝️ 문제 목록을 먼저 적었음

불편한 걸 표로 쭉 적어봤음.

| 상황 | 문제 |
|---|---|
| Cursor 쓰다 Claude Desktop 열면 | 맥락 처음부터 다시 설명 |
| 코드 에디터 + 영상 생성 동시 실행 | GPU 충돌, 팬 폭발 |
| AI 에이전트 여러 개 돌리면 | 서로 대화 못 함, 내가 중계해야 함 |
| MCP 서버 추가하려면 | JSON 설정, 포트, 재시작 — 개발자만 가능 |
| 스킬 40개 설치 | 매 대화마다 20,000토큰을 도구 설명에 씀 |
| 간단한 질문에 Claude API 호출 | 비용 낭비 — 로컬 소형 모델이면 충분 |
| AI가 내 파일 건드렸는지 | 알 방법 없음 |

적어놓고 보니 **근원이 하나였음.**

> AI 도구들이 각자의 섬이다.
> 같은 사용자, 같은 하드웨어인데 — 서로 모르고, 맥락을 공유하지 않고, 자원을 나누지 않는다.

## 🧵 이름은 seam

**seam** — 이음새, 솔기.
천을 꿰매는 실처럼 AI 앱들 사이에 투명하게 존재하면서 연결한다는 뜻.

```
code editor | video generator | planner | ai agent | ...
────────────────────────────────────────────────────────  ← seam
   ollama | claude api | openai api | gemini | ...
```

**seamless** 에서 따온 것도 있음. 사용자가 이음새의 존재를 못 느끼는 게 목표라서.

처음엔 "AI OS"라는 이름으로 기획했었는데(v0.1~v0.3), OS라고 하니까 너무 거창하고 실제로 하는 일이랑도 안 맞았음.
커널이나 스케줄러를 만드는 게 아니라 **그냥 중간에 끼는 레이어**임.

## 💡 핵심 아이디어 5개

### 1. 통합 메모리 — 모든 AI가 나를 기억함

모든 AI 앱과의 대화를 하나의 저장소로 모음.

```
어제 code editor에서: "TypeScript로 인증 API 만들었음, RS256 선택"
오늘 AI planner에서: "그 API에 맞는 프론트 일정 짜줘"
                     → seam이 "TypeScript, RS256 API" 맥락 주입
                     → planner는 처음 보는 대화인데 맥락을 알고 있음
```

### 2. LLM 추상화 — 모델을 몰라도 됨

요청의 복잡도랑 도메인 보고 seam이 고름.

```
간단한 질문  → 로컬 소형 모델 ($0)
코딩 작업    → 로컬 코딩 특화 모델 ($0)
복잡한 분석  → Claude API ($)
```

### 3. 스킬 지연 로딩 — 필요할 때만 적재

40개 깔아도 대화 시작할 때 토큰을 안 씀. 요청 보고 필요한 것만 그 순간에 로드.

### 4. 에이전트 통신 — AI들끼리 협업

```
code agent  →→→  seam 통신 버스  →→→  test agent
                                 →→→  deploy agent
```

내가 중계자가 될 필요가 없어짐.

### 5. 메모리 자동 업데이트

대화 끝날 때 "이 대화에서 기억해야 할 게 뭔지" seam이 판단해서 저장.

## 🏗️ 6레이어로 쌓았음

```
Layer 5 · UX             CLI · Activity Journal · REST API
Layer 4 · Memory Kernel  Working · Episodic · Semantic
Layer 3 · Skill Router   Lazy Loading · 트리거 매칭
Layer 2 · Trust Sandbox  Permission · 시스템 감시
Layer 1 · Smart Router   복잡도×도메인 라우팅 · 비용 예산
Layer 0 · Gateway        OpenAI/Anthropic/Ollama 호환 프록시
```

**Layer 0이 제일 중요함.** 기존 앱들이 seam을 거치게 만드는 게 여기서 결정됨.

```bash
export OPENAI_API_BASE=http://localhost:11400/v1
export ANTHROPIC_API_URL=http://localhost:11400/anthropic
export OLLAMA_HOST=http://localhost:11400/ollama
```

환경변수만 바꾸면 **앱은 자기가 프록시를 거치는지도 모름.**

## 🎣 2티어 — Tier 1은 미끼, Tier 2가 본체

이게 채택 전략의 핵심임.

**Tier 1 (프록시)**: 쓰던 앱 그대로 씀. seam이 사이에 껴서 메모리 주입하고 기록함.
가입 저항이 0임. 설치하고 환경변수만 바뀌면 끝.

**Tier 2 (네이티브)**: `seam "요청"` 으로 직접 씀. 전체 파이프라인을 다 씀.

| 레이어 | Tier 1 | Tier 2 |
|---|:-:|:-:|
| Gateway | ✅ | ✅ |
| Smart Router | △ 보수적 | ✅ |
| Trust Sandbox | △ 기록만 | ✅ |
| Skill Router | ✗ | ✅ |
| Memory | ✅ 주입만 | ✅ 주입+업데이트 |

```
Phase 1: Tier 1 메모리 공유   → "오 이게 되네?"
Phase 2: 비용 절감            → "진짜 돈이 줄었다"
Phase 3: seam CLI 직접 사용   → "이게 더 편하네"
Phase 4: 신뢰 + 자동화        → 정착
```

## 🛡️ seam이 죽어도 앱은 돌아가야 함

설계하면서 제일 신경 쓴 부분임.

모든 AI 요청이 seam을 거치게 만들면, **seam이 단일 장애점**이 됨.
게이트웨이가 죽으면 Cursor도 못 쓰고 Claude Code도 못 씀. 그건 최악임.

그래서 passthrough 폴백을 설계에 박아뒀음.

```
Gateway 정상 → 모든 기능
Gateway 장애 → 환경변수를 원래 API 엔드포인트로 자동 복원
              앱들은 직접 API와 통신 (메모리·라우팅 없이)
              seam 재시작 시 자동 복귀
```

사용자한테 보장하는 건 한 줄임. **"seam이 죽어도 AI 앱은 동작한다."**

## ❓ 열린 질문을 같이 적었음

모르는 걸 모른다고 적어두는 게 나중에 도움이 될 것 같았음.

- Gateway 장애 시 passthrough 폴백을 **구체적으로 어떻게** 구현하나 (iptables? 로컬 캐시?)
- 복잡도 분류: 규칙 기반 vs 경량 분류 모델 vs 하이브리드
- 메모리 주입 threshold 0.75 — 낮으면 노이즈, 높으면 누락
- 에이전트 버스 프로토콜: 자체 설계 vs 기존 메시지 브로커
- 오픈소스 전략: 코어 OSS + 클라우드 메모리 동기화 유료?

## 🎯 느낀 점

하루 종일 문서만 10개 썼는데 코드보다 이게 더 힘들었음.

제일 오래 붙든 건 **"이걸 왜 쓰지?"** 였음.
기능을 나열하는 건 쉬운데, 사용자가 설치할 이유 하나를 만드는 게 어려웠음. Tier 1을 미끼로 두는 구조가 그 답이었음.

근데 이 설계가 일주일 뒤에 **통째로 갈아엎힘.** 다음 글에서.

#seam #AI #설계 #아키텍처 #LLM
