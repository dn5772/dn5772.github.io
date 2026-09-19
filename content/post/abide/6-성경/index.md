---
title: "ABIDE 개발 일지 #6 - 성경 버전을 늘리니 UI가 줄줄이 깨졌음"
description: "KJV/KRV/ASV 다버전 지원. 그리고 키보드가 입력창을 가리는 버그들"
slug: "abide-6"
date: 2026-03-27T17:47:00+09:00
draft: false
tags: ["ABIDE", "Flutter", "UI", "버그", "다국어", "모바일"]
categories: ["app/abide"]
---

# ABIDE 개발 일지 #6 - 성경 버전을 늘리니 UI가 줄줄이 깨졌음

[#1]({{< ref "/post/abide/1-앱-뼈대" >}})에서 성경 본문을 **로컬 에셋**으로 넣었음. 개역한글 하나만.
영어 사용자를 생각하면 버전이 여러 개여야 함.

3월 23일 새벽에 이걸 붙였는데, **한 시간 동안 커밋 15개**가 나왔음. 대부분 버그 수정임.

## 📖 에셋을 권 단위로 쪼갬

```
01:13  feat: add KJV/KRV/ASV per-book Bible asset files
01:20  feat: reorganize bible assets into version folders (NKRV)
```

원래 성경 전체가 파일 하나였음. 버전이 셋이 되면 그게 셋이 됨.

**권 단위로 쪼갰음.**

```
assets/bible/KRV/genesis.json
assets/bible/KJV/genesis.json
assets/bible/ASV/genesis.json
```

창세기를 읽는데 요한계시록까지 메모리에 올릴 이유가 없음.
[#1]({{< ref "/post/abide/1-앱-뼈대" >}})에서 만든 로컬 파서가 권 단위 인덱싱을 하고 있어서 쪼개는 건 어렵지 않았음.

루트 레포에도 스크립트를 넣었음.

```
feat: add Bible split script and KJV/KRV/ASV per-book assets
```

## 🔗 버전 하나 추가에 파일 여섯 개

여기서부터가 진짜임. `BibleVersion` 이라는 값 하나가 생기니까 **계층을 전부 통과해야 했음.**

```
01:26  feat: add English fields to BibleBookMeta for multi-version support
01:30  feat: add BibleSettingsProvider with version/style/fontSize
01:37  feat: add version parameter to BibleService methods
01:37  fix: correct import path in bible_service.dart
01:41  fix: pass BibleVersion to BibleService in BibleRepositoryImpl
01:59  feat: pass BibleVersion to loadBook calls in BibleChapterScreen
```

```
BibleSettingsProvider  →  BibleRepositoryImpl  →  BibleService  →  asset 경로
      (presentation)            (data)              (data)
```

[#1]({{< ref "/post/abide/1-앱-뼈대" >}})에서 3레이어로 가른 대가임. **파라미터 하나가 레이어를 다 뚫고 내려가야 함.**

커밋 두 개(`01:37`, `01:41`)가 연달아 "인자 안 넘김"을 고치는 건데, 컴파일은 되는데 기본값으로 동작해서 화면에서만 티가 났음.

솔직히 이 부분은 레이어를 나눈 게 손해였음. 근데 **책 이름 영문 필드**(`01:26`)처럼 도메인 엔티티만 고치면 되는 건 또 깔끔했음. 반반임.

## ⚙️ 설정 시트

```
01:42  feat: add BibleSettingsSheet bottom sheet
01:44  fix: use design system tokens in BibleSettingsSheet
01:47  feat: add settings button and sequential listing to BibleIndexScreen
```

버전·글꼴 크기·본문 스타일을 바꾸는 바텀시트임.

`01:44` 커밋이 [#1]({{< ref "/post/abide/1-앱-뼈대" >}})에서 디자인 시스템을 먼저 깐 값어치임.
처음엔 색을 직접 박았다가 **토큰으로 바꿈.** 다크 모드에서 안 맞았음.

## 🐛 그리고 버그가 줄줄이

여기서부터 나흘 동안 UI 버그만 잡았음.

```
03-23 01:49  fix: show English book name in continue-reading card when English version active
03-23 01:54  fix: meditation page verse state, overlay close button, keyboard overlap, remove resume banner
03-23 10:13  fix: 5가지 UI 버그 수정
03-23 20:46  fix: 묵상 스크린 오버레이가 터치 이벤트를 차단하는 버그 수정
03-26 13:28  fix: 선택 모드 + 권 셀렉터 동시 오픈 시 오버플로우 수정
03-26 13:40  fix: 홈 화면 감사 무드 이모지 누락 수정
03-26 13:57  fix: 성경 설정 SegmentedButton 선택 상태 글씨색 흰색으로 수정
03-26 14:04  fix: 성경 설정 시트 배경색 명시해 텍스트 가독성 개선
03-26 14:22  fix: 성경 미리보기 overlay가 키보드 오픈 시 입력창 가리는 버그 수정
03-27 12:14  fix: 성경 글자 크기 설정이 본문/미리보기에 실제로 반영되도록 수정
03-27 17:47  fix: 성경 설정 시트 폰트 크기 확대 시 bottom overflow 수정
```

**11개 중 7개가 설정 시트 하나에서 나왔음.**

패턴이 보임.

**1. 오버레이 겹침.** 묵상 화면에서 구절 미리보기를 띄우면 **키보드가 올라올 때 입력창을 가림.**
모바일에서 오버레이 + 키보드는 거의 항상 문제가 됨. 화면 높이가 실시간으로 바뀌는데 오버레이는 그걸 모름.

**2. 터치 차단.** 투명한 오버레이가 **보이지도 않으면서 터치를 먹고 있었음.**
화면은 정상인데 버튼이 안 눌림. 제일 찾기 어려운 종류임.

**3. 글자 크기가 반영이 안 됨.** 설정에서 키우면 값은 바뀌는데 **본문에 안 닿음.**
프로바이더까지는 갔는데 위젯이 그 값을 구독하지 않고 있었음.

**4. 크기를 키우면 오버플로우.** 반영되게 고쳤더니 이번엔 시트가 터짐.
`01:44` 에서 고정 높이로 잡아둔 게 원인. 스크롤 가능하게 바꿈.

## 🎨 이모지를 아이콘으로 바꿈

25일에 이걸 했음.

```
refactor: replace emoji with Material Design icons throughout UI
```

무드 선택이나 빠른 진입 버튼에 이모지를 쓰고 있었음.

이모지는 **기기·OS 버전마다 그림이 다름.** 같은 유니코드인데 안드로이드랑 iOS가 완전히 다르게 그림.
디자인을 맞출 수가 없고, 폰트가 없으면 두부(□)가 나옴.

실제로 `03-26 13:40` 에 "감사 무드 이모지 누락"이 있음. 아이콘으로 바꾼 뒤에도 한 군데가 남아 있었던 것.

같은 날 이것도 같이 고침.

```
fix: restrict debug logging to debug builds, fix quota API and notification settings parsing
```

**디버그 로그가 릴리즈 빌드에도 찍히고 있었음.** 토큰이나 응답 본문이 로그에 남음.
[#4]({{< ref "/post/abide/4-인증" >}})에서 민감 정보 마스킹한 거랑 같은 줄기인데, 이번엔 클라이언트 쪽이었음.

## 🎯 느낀 점

**"버전 하나 추가"가 UI 버그 11개를 불렀음.**

기능 자체는 반나절이면 되는데, 그게 건드리는 화면이 성경 인덱스·본문·설정 시트·묵상 화면·홈 카드까지 퍼져 있었음.
**옵션이 하나 늘면 조합이 곱으로 늘어난다**는 걸 몸으로 배웠음.

그리고 모바일에서 **오버레이랑 키보드는 항상 의심**해야 함. 11개 중 3개가 그거였음.

다음은 스트리밍이랑 Observer 깊이 판정임. 이 시리즈에서 제일 오래 붙든 부분.

#ABIDE #Flutter #UI #버그 #모바일
