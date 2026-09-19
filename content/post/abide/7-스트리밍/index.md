---
title: "ABIDE 개발 일지 #7 - AI가 너무 빨리 '깊어졌다'고 판단했음"
description: "토큰 단위 실시간 스트리밍, Observer 깊이 점수 캡, 그리고 사용자가 거절했는데 또 묻는 버그"
slug: "abide-7"
date: 2026-03-28T20:38:00+09:00
draft: false
tags: ["ABIDE", "LangGraph", "SSE", "스트리밍", "AI에이전트", "캐시"]
categories: ["app/abide"]
---

# ABIDE 개발 일지 #7 - AI가 너무 빨리 '깊어졌다'고 판단했음

[#3]({{< ref "/post/abide/3-에이전트" >}})에서 만든 Observer 노드를 이때 제대로 고쳤다.
이 시리즈에서 제일 오래 붙든 부분이다.

## 먼저 토큰 단위 스트리밍

```
03-25 22:07  feat: 묵상 응답을 astream_events로 실시간 토큰 스트리밍
```

원래는 **노드가 끝나야** 응답이 나갔다. LangGraph가 Counselor 노드를 다 돌리고 나서 결과를 한 번에 보낸다.

[#1]({{< ref "/post/abide/1-앱-뼈대" >}})에서 SSE를 깔아놨는데도 **체감이 로딩 스피너랑 똑같았다.**
2~3초 멈춰 있다가 문단이 통째로 툭 나온다.

LangGraph의 `astream_events` 로 바꿔서 **LLM 토큰이 나오는 대로** SSE로 흘려보냈다.

```
LLM 토큰 → astream_events → SSE delta → Flutter SseClient → 화면
```

같은 응답인데 **대화처럼 보이기 시작한다.** 코드는 노드 하나 바꾼 건데 인상이 완전히 달라졌다.

## Observer가 너무 후했음

[#3]({{< ref "/post/abide/3-에이전트" >}})에 적었듯 Observer가 "지금 얼마나 깊이 들어왔나"를 점수로 매기고, Supervisor가 그걸 보고 마무리할지 정한다.

문제는 **두세 마디 만에 점수가 확 올라갔다.**

사용자가 좀 성의 있게 답하면 Observer가 "오 깊어졌네" 하고 높은 점수를 준다.
그러면 Supervisor가 마무리로 간다. **대화가 3턴 만에 끝난다.**

묵상인데 3턴이면 묵상이 아니다.

```
03-28 18:55  feat: observer prompt — turn-aware depth scoring caps
03-28 19:28  feat: observer — turn-aware delta cap prevents early over-scoring
03-28 19:35  fix: observer depth non-decreasing invariant + accurate comment + more tests
```

세 커밋에 걸쳐 고쳤다.

**1. 턴 수를 알려준다.** Observer가 자기가 몇 번째 턴인지 모르고 있었다. 지금 대화 내용만 보고 점수를 매긴다.
한 마디만 보면 그럴듯한데, **2턴째에 만점이 나올 수는 없다.** 턴 수에 따라 점수 상한을 걸었다.

**2. 한 번에 뛸 수 있는 폭을 제한.** 상한만으로는 부족했다. 3→8처럼 한 턴에 확 뛰면 상한 안이어도 이상한다.
**델타 캡**을 걸어서 한 턴에 올라갈 수 있는 폭을 묶었다.

**3. 깊이는 줄어들지 않게.** 이게 마지막에 발견한 건데, 점수가 **내려가는** 경우가 있었다.
사용자가 짧게 답하면 Observer가 이전보다 낮게 준다. 그러면 마무리로 가다가 다시 되묻기로 돌아온다.

깊이는 **누적 개념**이라 줄어들면 안 된다. 한 번 8까지 간 대화가 7로 내려갈 수는 없다.
`max(이전, 이번)` 으로 단조 증가를 강제하고 테스트를 붙였다.

## "아니요" 했는데 또 묻는 버그

```
03-28 19:38  feat: depth_end_declined guard — no repeated confirm_end after user declines
03-28 19:44  fix: turn_count>=8 confirm_end also respects depth_end_declined guard
```

ConfirmEnd 노드가 "이제 마무리할까요?"라고 묻는데, 사용자가 "아니요"라고 하면 **다음 턴에 또 물었다.**

깊이 점수는 이미 높은 상태니까 Supervisor가 계속 ConfirmEnd를 고른다. 사용자는 두 턴마다 "그만할까요?"를 들는다.

`depth_end_declined` 플래그를 상태에 넣어서, 한 번 거절하면 **깊이 기반 종료 제안을 더 안 하게** 막았다.

19:44 커밋이 6분 뒤인데, **종료 제안 경로가 두 개**였다.
깊이 기반 말고 `turn_count >= 8` 로도 들어가는 길이 있었는데 거기는 가드를 안 봄. 같은 가드를 붙였다.

**조건을 막을 땐 그 조건으로 들어오는 경로가 몇 개인지 세야** 한다.

## 묵상 노트를 별도 이벤트로

```
03-28 19:48  feat: scribe sends brief closing message — full note via meditation_note SSE event
03-28 20:00  feat: send dedicated meditation_note SSE event; remove from done payload
03-28 20:06  fix: normalize key_insights with _normalize_application in meditation_note event
03-28 20:33  fix: add is_final to meditation_start done event for consistency with chat
03-28 20:38  (flutter) feat: parse meditation_note SSE event; fix message handler for non-streamed content
```

원래 Scribe가 묵상 노트를 **채팅 메시지로** 보냈다. 대화 말풍선 안에 정리 노트가 통째로 들어간다.

이게 이상했던 게, 노트는 **대화가 아니라 결과물**이다. 저장하고 나중에 볼 거라 말풍선에 있으면 안 된다.

그래서 갈랐다.

```
Scribe  →  짧은 마무리 인사        → 채팅 메시지
        →  묵상 노트 전문          → meditation_note SSE 이벤트
```

Flutter 쪽도 같이 고쳤다. 이 이벤트를 받아서 **노트 화면으로** 넘긴다.

20:33 커밋이 자잘한데 중요하다. `meditation_start` 의 done 이벤트에만 `is_final` 이 없었다.
**같은 의미의 필드가 엔드포인트마다 다르면** 클라이언트가 분기를 두 벌 짜야 한다.

## DeepLens를 미리 계산해둠

같은 기간에 DeepLens 쪽도 크게 바뀌었다.

```
03-26 00:51  feat: add DeepLens pre-computation seed script
03-27 00:03  refactor: rewrite seed_deeplens.py for parallel chunked processing
03-27 17:01  (core) 딥렌즈 분석 요청 기능 개선: DB 캐시 우선 조회 추가 및 캐시 미스 시 AI 서버로 폴백
```

DeepLens는 구절 하나의 원어·배경·상호참조를 분석하는 기능인데, **매번 LLM을 부르면 느리고 비싼다.**

그런데 성경 구절은 **개수가 정해져 있다.** 사용자가 뭘 고를지 몰라도 **전부 미리 계산해둘 수 있다.**

결국 **62,189개 구절(KRV+NIV)을 사전 계산**해서 DB에 넣었다.
시드 스크립트를 병렬 청크 처리로 다시 쓴 게 27일 새벽 커밋이다. 순차로 돌리면 며칠 걸린다.

Core 서버는 이제 **DB를 먼저 보고**, 없을 때만 AI 서버로 간다.

```
Flutter → Core → deep_lens_cache 조회 → 있으면 즉시 반환
                                      → 없으면 AI 서버 폴백
```

[#3]({{< ref "/post/abide/3-에이전트" >}})에서 200회로 잡았던 쿼터를 이때 올렸다. 캐시 히트면 비용이 거의 0이라서.

**"매번 생성"을 "미리 다 만들어둠"으로 바꾼 것**인데, 입력 공간이 유한할 때만 되는 방법이다. 여기선 딱 맞았다.

## 느낀 점

**LLM한테 숫자를 매기게 하면 캘리브레이션이 안 된다.**

Observer가 "깊이 점수"를 주는데, LLM은 절대 기준이 없다. 지금 보는 텍스트 안에서만 판단한다.
그래서 턴 수를 알려주고, 상한을 걸고, 델타를 묶고, 단조성을 강제하는 **네 겹의 가드**가 필요했다.

결국 **판정은 LLM이 하고 제약은 코드가 건다**는 형태로 정리됐다. [#3]({{< ref "/post/abide/3-에이전트" >}})에서 wrap_up 무한 반복을 카운터로 막은 거랑 같은 결론이다.

그리고 `depth_end_declined` 가드를 두 경로에 따로 붙여야 했던 게 기억에 남는다.
**상태 플래그를 추가할 땐 그걸 읽어야 하는 곳을 전부 찾아야** 하는데, 그래프 구조에서는 그게 눈에 잘 안 보다.
