---
title: "정담 개발 일지 #1 - 템플릿 하나로 시작했음"
description: "create-next-app 치고 하루 만에 식당 홈페이지 뼈대 세우기. 850줄짜리 page.js를 컴포넌트 12개로 쪼갬"
slug: "jung-dam-1"
date: 2026-01-15T16:20:00+09:00
draft: false
tags: ["정담", "웹개발", "NextJS", "React", "템플릿", "리팩터링"]
categories: ["web/jung-dam"]
---

# 정담 개발 일지 #1 - 템플릿 하나로 시작했음

식당 홈페이지를 만들게 됐음. 이름은 **정담**.

- 사이트: <https://jung-dam.vercel.app>
- 코드: <https://github.com/dn5772/jung_dam>

오늘 커밋 6개 찍었는데 전부 1월 15일 하루임. 아침 8시 44분에 `create-next-app` 치고 저녁까지 달림.

## 🎨 디자인은 안 만들기로 함

식당 홈페이지는 디자인이 반인데, 난 디자이너가 아님.
그래서 BootstrapMade의 **Yummy** 템플릿을 사서 Next.js로 옮기기로 했음.

이게 맞는 판단이었던 게, 식당 홈페이지에 필요한 섹션이 이미 다 들어 있음.

```
Hero · About · Why Us · Stats · Menu · Testimonials
Chefs · Events · Gallery · Book a Table · Contact
```

직접 짰으면 이 목록 만드는 데만 며칠 걸렸을 것. 없는 섹션(Chefs, Events)은 나중에 빼거나 바꾸면 되고.

## 🔨 정적 HTML을 Next.js로 옮기기

템플릿은 그냥 HTML + Bootstrap + jQuery 계열 라이브러리 뭉치임. 이걸 App Router에 얹는 게 오늘 작업의 대부분이었음.

`4d7b5e6` 커밋이 이럼.

```
43 files changed, 2810 insertions(+), 101 deletions(-)
src/app/css/main.css        1650 ++++++++++++++++
src/app/page.js              850 ++++++++++
src/components/YummyScripts.js 201 ++++
```

**`main.css` 1650줄은 손 안 대고 통째로 가져왔음.** 여기서 뭘 아끼려고 하면 디자인이 깨짐.

문제는 스크립트임. 템플릿은 `window.onload` 에서 AOS 초기화하고, 모바일 네비 토글 붙이고, 스크롤 이벤트 걸고 하는 걸 전제로 짜여 있음. React에선 그게 그대로 안 됨.

그래서 `YummyScripts.js` 라는 클라이언트 컴포넌트 하나를 만들어서 **그 초기화 로직을 전부 여기로 몰아넣음.** AOS, 모바일 네비, 스크롤 동작.
Bootstrap JS랑 `purecounter_vanilla.js` 는 Next의 `<Script strategy="afterInteractive">` 로 따로 실어줌.

## ✂️ 850줄짜리 page.js 쪼개기

옮기고 나니까 `page.js` 하나가 850줄이 됨. 스크롤이 안 끝남.

그래서 같은 날 바로 섹션별 컴포넌트로 쪼갰음.

```
src/app/page.js                        691 --------------
src/components/MenuSection.js          324 ++++++++++++
src/components/ContactSection.js        80 ++++
src/components/EventsSection.js         46 ++
src/components/GallerySection.js        39 ++
src/components/TestimonialsSection.js   38 ++
src/components/StatsSection.js          35 ++
src/components/ChefsSection.js          32 ++
src/components/AboutSection.js          31 ++
src/components/WhyUsSection.js          24 ++
src/components/BookATableSection.js     22 ++
src/components/HeroSection.js           21 ++
```

`page.js` 에서 691줄이 빠지고 컴포넌트 11개가 생김. `page.js` 는 이제 섹션을 순서대로 나열하는 것만 함.

**`MenuSection.js` 가 324줄로 혼자 제일 큼.** 이때는 몰랐는데 앞으로 이 파일을 제일 많이 건드리게 됨.

## 🐛 오늘 고친 잔버그

```
🐛 fix: fix the detail, info
🐛 fix: fix menu nav bar
feat: ubdate hero      ← 오타 남
```

네비게이션 바가 섹션 앵커랑 안 맞아서 엉뚱한 데로 점프하는 것, hero 문구 교체 같은 거.
커밋 메시지에 `ubdate` 오타 난 거 지금 보니까 웃김.

## 🎯 느낀 점

**디자인을 안 만들기로 한 게 오늘의 전부인 듯.** 템플릿 값이 몇 만원인데 며칠을 벌었음.

정적 템플릿을 React로 옮길 때 제일 성가신 건 CSS가 아니라 **초기화 스크립트**였음.
"페이지 다 뜨면 실행돼야 하는 코드"를 어디에 둘지가 문제인데, 클라이언트 컴포넌트 하나에 몰아넣는 걸로 일단 정리함.

다음엔 메뉴 데이터 붙일 차례임.

#정담 #웹개발 #NextJS #React #템플릿
