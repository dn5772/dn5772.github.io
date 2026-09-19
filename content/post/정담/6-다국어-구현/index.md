---
title: "정담 개발 일지 #6 - 다국어 구현이랑 Analytics"
description: "API에 locale 쿼리 붙이고, 관리자에서 두 언어 동시 편집. 그리고 방문자 수를 처음 봄"
slug: "jung-dam-6"
date: 2026-02-04T16:36:00+09:00
draft: false
tags: ["정담", "웹개발", "다국어", "i18n", "React", "Analytics"]
categories: ["web/jung-dam"]
---

# 정담 개발 일지 #6 - 다국어 구현이랑 Analytics

[#5]({{< ref "/post/정담/5-다국어-설계" >}})에서 정한 설계대로 구현한다. 2월 2~4일 커밋 9개.

## API에 locale 붙이기

`GET` 은 쿼리 파라미터 하나 추가하면 끝이다.

```js
const locale = searchParams.get('locale') || 'ko';
const validLocales = ['en', 'ko'];
const safeLocale = validLocales.includes(locale) ? locale : 'ko';
const fileName = `menuData-${safeLocale}.json`;
```

**허용 목록으로 검증하는 게 중요하다.** 이게 파일명에 그대로 들어가니까, 검증 없으면 `?locale=../../something` 같은 게 들어올 수 있다.

캐시 헤더도 같이 달았다.

```js
'Cache-Control': 'public, s-maxage=60, stale-while-revalidate=300'
```

메뉴는 한 달에 몇 번 바뀌는데 조회는 계속 일어난다. CDN이 60초 캐시하고, 만료돼도 5분 동안은 **일단 옛날 걸 주면서 뒤에서 갱신**한다.
사장님이 메뉴를 고치면 최대 1분 늦게 반영되는데, 그 정도는 괜찮다고 본다.

## LanguageContext

언어 상태는 컨텍스트 하나로 관리한다.

```
localStorage('locale')  →  navigator.language  →  'ko'
```

저장된 선택이 있으면 그거, 없으면 브라우저 언어, 그것도 아니면 한국어.
**기본을 영어가 아니라 한국어로 둔 건 손님 대부분이 한국인이라서다.**

`MenuSection` 은 `/api/menu?locale=<locale>` 을 부르고, 언어가 바뀌면 다시 가져온다.

한 가지 좋았던 게, API가 이미 단일 언어 데이터를 주니까 **화면 코드에서 언어 분기가 통째로 사라졌다.**

```jsx
// 전
<h4>{getLocalizedValue(item.title, currentLocale)}</h4>

// 후
<h4>{item.title}</h4>
```

[#5]({{< ref "/post/정담/5-다국어-설계" >}})에서 파일을 쪼개기로 한 이득이 여기서 나온다.

## 관리자는 두 언어를 동시에

관리자 화면에선 반대로 합쳐서 다룬다. 서버에서 받은 ko/en 두 파일을 하나로 병합해서 편집하고, 저장할 때 다시 쪼갠다.

```
불러오기:  menuData-ko.json + menuData-en.json  →  { title: { ko, en }, ... }
저장:      { title: { ko, en }, ... }            →  { ko: {...}, en: {...} }
```

`PATCH` 도 양쪽 언어를 같이 받게 바꿨다.

```js
{
  action: 'add',
  categoryId: 'fried',
  data: {
    en: { title: 'Garlic Soy Chicken', ingredients: '...', price: '$18', image: '...' },
    ko: { title: '간장마늘치킨',        ingredients: '...', price: '$18', image: '...' }
  }
}
```

**한쪽만 보내는 경로를 안 만들었으니 어긋날 수가 없다.** 설계 때 정한 그대로다.

## Vercel Analytics

```
feat: add analytics integration to RootLayout component
feat: add @vercel/analytics dependency
```

`<Analytics />` 한 줄 넣으면 끝이다. 설정 없다.

이거 붙이고 나서 처음으로 **사람들이 실제로 뭘 보는지** 알게 됐다.
대부분 모바일이고, 메뉴 섹션에 제일 오래 머무른다. [#2]({{< ref "/post/정담/2-메뉴" >}})에서 만든 "메뉴로 바로 가기" 버튼이 괜히 만든 게 아니었다.

이 관찰이 나중에 [#8]({{< ref "/post/정담/8-모바일" >}})에서 관리자 화면을 모바일 우선으로 갈아엎는 근거가 된다.

## 브랜치를 나눠서 작업함

이번엔 PR을 세 개 만들었다.

```
Merge pull request #1 from dn5772/devlop
Merge pull request #2 from dn5772/feat/localization
Merge pull request #3 from dn5772/devlop
```

혼자 하는 프로젝트라 브랜치가 의미 있나 싶었는데, **다국어 작업 중에 메뉴가 깨지면 손님한테 바로 보다.**
`main` 에 바로 밀지 않고 Preview 배포에서 확인하고 머지하니까 마음이 편했다.

## 느낀 점

**설계를 먼저 해두니까 구현이 그냥 받아쓰기였다.** Phase 2~4가 문서에 적힌 순서대로 끝난다.
[#4]({{< ref "/post/정담/4-저장소" >}})처럼 배포하고 나서 뒤집는 일이 한 번도 없었다.

Analytics는 진작 붙일 걸 그랬다. 한 줄인데 "내 추측"이랑 "실제 손님 행동"을 구분해준다.
