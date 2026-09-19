---
title: "seam 개발 일지 #6 - 비용과 품질을 슬라이더 하나로"
description: "0부터 9까지. 로컬 Ollama가 복잡도를 판정하고, 그 결과로 모델을 고름"
slug: "seam-6"
date: 2026-03-26T22:44:00+09:00
draft: false
tags: ["seam", "라우팅", "Ollama", "LLM", "비용", "메모리"]
categories: ["tool/seam"]
---

# seam 개발 일지 #6 - 비용과 품질을 슬라이더 하나로

[#1]({{< ref "/post/seam/1-왜" >}})의 핵심 아이디어 5개 중 두 번째가 **"LLM 추상화. 사용자는 모델을 몰라도 된다"** 였다.

말은 좋은데 막상 구현하려니까 문제가 있다. **사용자가 모델을 모르면, 비싼 모델을 써도 모른다.**
알아서 골라주는 게 좋다가도, 청구서를 보면 얘기가 달라진다.

그래서 **슬라이더 하나**로 풀었다.

## 0에서 9까지

```
seam setup  →  라우팅 성향: [0 ────●──── 9]
```

| 슬라이더 | 전략 |
|---|---|
| **0–2** | 로컬 Ollama 최우선. 부족하면 최저가 API (haiku / gemini-flash / gpt-4o-mini) |
| **3–5** | 균형. 복잡도 판정 후 simple→경량 API, complex→중급 API |
| **6–8** | 성능 우선. complex 요청은 최고 모델 (opus / gemini-pro / gpt-4o) |
| **9** | 항상 최고 모델 |

"어떤 모델 쓸래?" 대신 **"돈이랑 품질 중에 어디에 가까울래?"** 를 묻는다.
모델 이름은 계속 몰라도 되는데, 비용 방향은 내가 쥐고 있다.

값은 `~/.seam/config.toml` 에 들어가고, 범위를 벗어나면 0~9로 잘라낸다.

```
22:29  fix: add Clone to SeamAppConfig, clamp router_slider to 0-9 on load
```

설정 파일은 사람이 손으로 고칠 수 있으니까 `router_slider = 42` 가 들어올 수 있다. 읽을 때 캡핑한다.

## 복잡도는 누가 판정하나

슬라이더만으론 부족하다. "이 요청이 간단한가 복잡한가"를 알아야 3~8 구간에서 갈래를 탈 수 있다.

근데 **복잡도를 판정하려고 GPT-4를 부르면 본말전도**이다. 판정 비용이 실행 비용보다 비쌀 수 있다.

그래서 **로컬 Ollama 소형 모델**한테 시킨다.

```
요청 텍스트 → Ollama (phi3:mini / llama3.2:3b) → simple | medium | complex
비용: $0
타임아웃: 5초
```

Ollama가 안 떠 있거나 느리면 **휴리스틱으로 폴백**한다. 텍스트 길이 기준.

```
22:20  feat: implement Ollama complexity classifier with heuristic fallback
```

이 폴백이 꼭 필요했던 게, **분류기가 전체 파이프라인의 단일 장애점이 되면 안 되기 때문**이다.
Ollama를 안 깐 사람도 seam을 쓸 수 있어야 한다. 라우팅 품질만 떨어지고 동작은 해야 한다.

[#1]({{< ref "/post/seam/1-왜" >}})에서 "Gateway가 죽어도 앱은 돌아가야 한다"고 적었던 거랑 같은 원칙이다.

## 프로바이더 4개

```
Anthropic · OpenAI · Google · Ollama
```

라우팅 표가 (복잡도 × 슬라이더) → 프로바이더 + 모델로 가는데, **선택된 프로바이더에 키가 없으면** 다음으로 넘어간다.

```
22:20  feat: add RouterState with 0-9 slider routing logic and classifier stub
22:20  fix: add Debug derives and suppress dead_code in router
03-27 01:39  feat: GoogleModels medium 필드 추가, route()에 Google fallback 통합
03-27 01:44  feat: proxy_handler — router 연동, 4개 프로바이더, shared memory 주입
```

Google을 나중에 끼워넣었는데, `medium` 등급 모델이 비어 있어서 폴백 체인이 중간에 끊겼다.
**세 단계(simple/medium/complex)를 프로바이더마다 다 채워야** 표가 완성된다.

## 메모리를 두 겹으로 나눴음

같은 날 밤에 메모리도 갈랐다.

```
episodic.jsonl   private  — 환경별(env_id) 파티션. "이 환경에서 한 일"
shared.jsonl     shared   — 글로벌. "툴들이 같이 아는 것"
```

[#1]({{< ref "/post/seam/1-왜" >}})에서 "code editor에서 한 작업을 planner가 안다"고 했던 게 이 `shared` 레이어다.
그냥 다 공유하면 될 것 같은데 그러면 안 되는 게, **환경마다 관련 없는 맥락이 섞이다.**

백엔드 환경에서 한 작업이 영상 편집 환경 프롬프트에 끼어들면 노이즈다. 토큰도 낭비고.

```
22:14  feat: add SharedStore for cross-tool shared memory layer
22:32  feat: extend IpcState with SharedStore + RouterState; add shared_memory handlers
22:41  feat: wire SharedStore + RouterState into gateway daemon (seam 0.6)
```

그래서 **저장할 때 어디에 넣을지를 고르게** 한다. IPC에 핸들러를 따로 뒀다.

```
memory_save          → private (episodic, env_id 파티션)
shared_memory_save   → shared (글로벌)
```

주입할 때는 둘을 합쳐서 하나의 컨텍스트로 만듦.

```
03-27 01:41  feat: inject에 format_combined_context() 추가 (episodic + shared 결합)
```

## 키체인 조회 두 번

```
22:44  fix: fetch openai key once and reuse instead of double keychain lookup
```

작은 건데, 요청 하나 처리하면서 **OS 키체인을 두 번 조회**하고 있었다.
macOS 키체인 접근은 생각보다 안 싸고, 프록시는 요청마다 도는 경로라서 그대로 지연이 된다.

## 느낀 점

**"알아서 해준다"에는 항상 통제 손잡이가 하나 필요한 것 같다.**

모델 선택을 완전히 자동화하면 편한데, 사용자가 통제감을 잃는다.
모델 이름을 다 보여주면 통제는 되는데 [#1]({{< ref "/post/seam/1-왜" >}})의 문제로 되돌아간다.

슬라이더 하나가 그 사이 절충이었다. **추상화는 유지하되, 사용자가 신경 쓰는 축(비용)만 노출.**

그리고 분류기를 로컬 모델로 돌린 게 좋았다. 판정이 공짜라서 **모든 요청에 다 걸 수 있다.**
API를 불렀으면 "복잡도 판정할 가치가 있는 요청인가"를 또 판단해야 했을 것.
