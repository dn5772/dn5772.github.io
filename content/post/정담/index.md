---
title: "정담 - 식당 홈페이지를 넉 달 동안 고쳐 쓴 기록"
description: "템플릿에서 시작해서 DB 없는 메뉴 관리 시스템까지. 커밋 39개로 돌아보는 삽질"
slug: "jung-dam"
date: 2026-05-19
draft: false
tags: ["웹개발", "정담", "Next.js", "VercelBlob", "다국어", "보안"]
categories: ["web"]
---

# 정담 - 식당 홈페이지를 넉 달 동안 고쳐 쓴 기록

식당 홈페이지 하나 만들었음. 이름은 **정담**.

- 사이트: <https://jung-dam.vercel.app>
- 코드: <https://github.com/dn5772/jung_dam>

1월 15일에 `create-next-app` 으로 시작해서 5월 19일 마지막 커밋까지 39개. 넉 달인데 중간중간 뜯어고친 게 많아서 정리해봄.

스택은 Next.js 15 App Router + React 19. **DB는 안 씀.**

## 🏗️ 1월 - 템플릿 갖다 붙이기

처음엔 BootstrapMade의 Yummy 템플릿을 Next.js로 옮기는 작업이었음.
Hero, About, Menu, Chefs, Gallery, Events, Contact… 섹션이 한 무더기.

이때 커밋들이 죄다 이런 식임.

```
🐛 fix: fix menu nav bar
🐛 fix: fix the detail, info
fix: image paths in MenuSection component
fix: update contact links to use telephone protocol
```

전화번호 링크를 `tel:` 프로토콜로 바꾼 건 의외로 중요했음. 식당 홈페이지는 폰으로 보는 사람이 대부분인데 번호를 눌러서 바로 전화가 걸려야 함.

## 💥 1월 28일 - 메뉴 데이터를 어디에 둘 것인가 (하루에 3번 바뀜)

제일 삽질한 날임. 커밋 3개가 같은 날에 연달아 있음.

```
75eec6d  Fix Vercel deployment: Change menu API to memory-based storage
a8fee7f  refactor: removing in-memory storage and using direct file access
21ffce2  feat: integrate Vercel Blob for menu data storage and image uploads
```

**1차 - 파일에 쓰기.** 처음엔 `src/data/menuData.json` 을 직접 읽고 씀. 로컬에선 완벽하게 잘 됨.
배포하니까 관리자 페이지에서 메뉴를 저장해도 아무 일이 안 일어남. **Vercel 서버리스는 파일시스템에 쓸 수가 없음.** 이걸 몸으로 배움.

**2차 - 메모리에 올리기.** 그래서 프로세스 메모리에 올려두고 거기서 읽고 쓰게 바꿈. 배포하니까 저장은 됨.
근데 조금 있다가 새로고침하면 원래대로 돌아옴. 람다가 죽었다 살아나면 메모리가 날아가니까 당연한 일이었음. 44줄 추가했다가 그대로 되돌림.

**3차 - Vercel Blob.** 결국 메뉴 데이터를 **Blob에 JSON 파일로** 올림.

```
menuData-ko.json
menuData-en.json
```

DB를 안 쓴 이유는 간단함. 메뉴판은 데이터가 수십 개고, 하루에 몇 번 바뀔까 말까임.
테이블 만들고 마이그레이션 관리할 이유가 없었음. 이미지도 어차피 Blob에 올려야 하니 저장소를 하나로 통일함.

## ⚡ 2월 - 낙관적 업데이트

관리자 페이지에서 메뉴를 고칠 때마다 Blob 왕복을 기다리니까 답답했음.
그래서 화면을 **먼저** 바꾸고 요청을 뒤에 보내게 함. 실패하면 되돌리고.

메뉴판 편집은 실패할 일이 거의 없는 작업이라 이쪽이 체감이 훨씬 좋았음.

## 🌏 2~3월 - 한국어/영어

메뉴 아이템 한 칸이 이렇게 생김.

```json
{ "name": { "ko": "냉면", "en": "Cold Noodles" } }
```

관리자 페이지는 항목별로 ko/en 필드를 같이 편집하고, 저장할 때 `{ en: {...}, ko: {...} }` 형태로 보내서 **두 로케일 파일을 한 번에 갱신**함.

보는 쪽은 `LanguageContext` 가 `localStorage` 에 현재 언어를 들고 있고, 없으면 브라우저 언어로 감지함. 기본은 한국어.
`MenuSection` 은 `/api/menu?locale=<locale>` 을 부르고 언어가 바뀌면 다시 가져옴.

## 🔒 3월 25일 - 보안 구멍 메우기

이날 커밋 하나가 제일 뜨끔했음.

```
fix: resolve critical security vulnerabilities in API routes
```

고친 게 이만큼임.

**1. JWT 시크릿이랑 bcrypt 해시가 코드에 박혀 있었음.** 전부 환경변수로 뺐음.
`JWT_SECRET`, `ADMIN_PASSWORD_HASH`, `BLOB_READ_WRITE_TOKEN` 셋 다 `.env` 로.

**2. 로그인 라우트가 비밀번호를 평문으로 로깅하고 있었음.** 디버깅한다고 넣어놓고 그대로 둔 것. 삭제.

**3. 업로드 엔드포인트에 인증이 아예 없었음.** 이게 제일 심각했음.
`POST /api/upload` 가 무방비라 누구나 내 Blob 스토리지에 파일을 올릴 수 있었음. JWT 검증 추가함.

**4. 메뉴 저장에 레이스 컨디션이 있었음.** 원래 저장 로직이 이랬음.

```
list → delete → put
```

지우고 나서 올리는 사이에 **메뉴가 빈 채로 응답되는 창**이 있었음. 손님이 하필 그 순간에 들어오면 메뉴판이 텅 비어 보임.
`put(..., { addRandomSuffix: false })` 으로 **같은 이름에 덮어쓰기**로 바꿔서 delete 단계를 아예 없앴음.

**5. en/ko 저장을 병렬로.** 순차로 두 번 왕복하던 걸 `Promise.all` 로 묶음.

직접 짠 코드에서 이런 게 나오니까 좀 부끄러웠는데, 그래도 배포 전에 찾은 게 어딤.

## 📱 3월 25일 - 관리자 화면을 모바일 우선으로

사장님이 폰으로 메뉴를 고침. 근데 관리자 페이지가 데스크탑 기준이었음.

슬라이드 네비게이션으로 바꾸고, ko/en 입력 칸을 섹션으로 접었다 펼 수 있게 함.
드래그 정렬은 DnD Kit 쓰는데 터치에서도 잘 동작해서 그대로 뒀음.

## 🐛 5월 19일 - 마지막 한 방

3월에 `addRandomSuffix: false` 로 바꿨는데, 5월에 갑자기 메뉴 저장이 다시 실패하기 시작함.

```
fix: add allowOverwrite to Vercel Blob put to fix menu save failures
```

Blob SDK가 올라가면서 **같은 경로에 덮어쓰려면 `allowOverwrite` 를 명시적으로 줘야** 하게 바뀌었음.
3월에 delete를 없앤 그 결정이 5월에 한 번 더 손이 간 셈인데, 그래도 레이스 컨디션으로 메뉴가 비는 것보단 나음.

## 🎯 느낀 점

**Vercel에선 파일에 못 쓴다.** 이걸 하루에 세 번 고쳐가며 배움. 머리로 아는 거랑 배포해서 깨지는 걸 보는 건 다름.

**DB 없이도 되는 게 꽤 많음.** 메뉴판처럼 작고 자주 안 바뀌는 데이터는 JSON 파일 두 장으로 충분했음. 오히려 스키마 마이그레이션이 없어서 편함.

**디버깅용 코드가 제일 위험함.** 비밀번호 평문 로깅이랑 인증 없는 업로드 엔드포인트, 둘 다 "일단 되게 해놓고 나중에" 하고 넘긴 것들이었음. 나중이 두 달 뒤였음..

#정담 #웹개발 #NextJS #VercelBlob #다국어 #보안
