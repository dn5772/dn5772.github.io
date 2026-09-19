---
title: "정담 개발 일지 #7 - 두 달 만에 보안 구멍 네 개를 찾았음"
description: "코드에 박힌 시크릿, 평문 비밀번호 로깅, 인증 없는 업로드, 메뉴가 잠깐 비는 레이스 컨디션"
slug: "jung-dam-7"
date: 2026-03-25T22:33:00+09:00
draft: false
tags: ["정담", "웹개발", "보안", "JWT", "레이스컨디션", "리팩터링"]
categories: ["web/jung-dam"]
---

# 정담 개발 일지 #7 - 두 달 만에 보안 구멍 네 개를 찾았음

두 달 만에 코드를 다시 열었음. [#3]({{< ref "/post/정담/3-관리자" >}})에서 하루에 1045줄 쏟아낸 그 관리자 API를 차분히 읽어봤는데,
**볼수록 등이 서늘해졌음.**

```
fix: resolve critical security vulnerabilities in API routes
 src/app/api/auth/login/route.js   60 +++-------
 src/app/api/menu/route.js        124 +++++-------------
 src/app/api/upload/route.js       31 ++++---
 3 files changed, 74 insertions(+), 141 deletions(-)
```

141줄 지우고 74줄 넣음. **고치면서 코드가 줄어든 게 좀 웃김.**

## 🔴 1. 시크릿이 코드에 박혀 있었음

로그인 라우트에 **JWT 시크릿이랑 bcrypt 해시가 그대로** 들어 있었음.

`.env` 를 만들기 전에 "일단 되게" 하려고 상수로 박아놓고, 동작하니까 그대로 넘어간 것.
공개 저장소라 그대로 올라가 있었음.

전부 환경변수로 뺐음.

```
JWT_SECRET
ADMIN_PASSWORD_HASH
BLOB_READ_WRITE_TOKEN
```

## 🔴 2. 비밀번호를 평문으로 로깅하고 있었음

```js
console.log('Login attempt:', password);   // ← 이게 남아 있었음
```

로그인이 왜 안 되는지 보려고 넣은 건데 안 지웠음.
Vercel 대시보드 로그에 관리자 비밀번호가 평문으로 쌓이고 있었던 것.

지웠음.

## 🔴 3. 업로드 엔드포인트에 인증이 아예 없었음

이게 제일 심각했음.

```
POST /api/upload   ←  아무 검증 없음
```

[#3]({{< ref "/post/정담/3-관리자" >}})에서 메뉴 API엔 `verifyToken` 을 붙였는데 **업로드만 빼먹었음.**
누구나 내 Blob 스토리지에 파일을 올릴 수 있는 상태였음. 용량은 내가 내고.

```js
const user = verifyToken(request);
if (!user) return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
```

관리자 화면에서 이미지 올릴 때 `Authorization` 헤더를 안 보내고 있어서 그것도 같이 고침.

## 🟠 4. 메뉴가 잠깐 비는 레이스 컨디션

이건 보안은 아닌데 더 무서웠음.

저장 로직이 이랬음.

```
list  →  del(기존 파일)  →  put(새 파일)
```

**지우고 나서 올리는 사이에 메뉴가 빈 채로 응답되는 창이 있었음.**
손님이 하필 그 0.x초에 들어오면 메뉴판이 텅 비어 보임. 에러도 아니고 그냥 빈 메뉴라 아무도 신고를 안 해줌.

`del` 단계를 아예 없애고 **같은 이름에 덮어쓰기**로 바꿨음.

```js
await put('menuData-ko.json', body, {
  token: BLOB_TOKEN,
  access: 'public',
  contentType: 'application/json',
  addRandomSuffix: false,   // ← 같은 경로에 덮어쓰기
});
```

`addRandomSuffix: false` 를 줘야 랜덤 접미사 없이 같은 경로를 씀. 그래야 덮어쓰기가 됨.

덤으로 ko/en 저장을 `Promise.all` 로 묶어서 왕복을 반으로 줄였음.
응답도 전부 `NextResponse.json` 으로 통일함. 원래는 `new Response(JSON.stringify(...))` 랑 섞여 있었음.

## 🐛 같은 날 잡은 버그 8개

보안 말고 커밋 하나 더 있음.

**`mergeLocalizedMenuData` 가 EN만 기준으로 합치고 있었음.**
한국어에만 있는 카테고리는 관리자 화면에 아예 안 뜸. 합집합으로 바꿈.

**편집 중에 배경 갱신이 폼을 덮어씀.**
`editItem` 이펙트 의존성에 `menuData` 가 들어 있어서, 백그라운드 리페치가 오면 **저장 안 한 입력이 날아감.** 의존성에서 뺐음.

**GLightbox 메모리 누수.**
언어를 바꿀 때마다 라이트박스를 새로 만드는데 기존 걸 안 없앴음. 언어 토글을 왔다갔다 하면 인스턴스가 계속 쌓임.
ref로 인스턴스를 들고 있다가 재생성 전에 `destroy()` 하고, 언마운트 때도 정리하게 함.

**첫 화면에서 언어가 깜빡였음.**
영어 사용자가 들어오면 한국어가 한 프레임 보였다가 영어로 바뀜. `localStorage` 를 `useEffect` 에서 읽어서 생긴 일.
`useState` 의 **lazy initializer** 로 옮겨서 첫 렌더 전에 동기적으로 읽게 함.

```js
// 전
const [lang, setLang] = useState('ko');
useEffect(() => { setLang(localStorage.getItem('locale') ?? detect()); }, []);

// 후
const [lang, setLang] = useState(() => localStorage.getItem('locale') ?? detect());
```

나머지는 중복 가드 제거, 쓸모없는 별칭 제거, 디버그 `console.log` 청소.
`html lang` 속성이 언어 따라 안 바뀌던 것도 고쳤음.

## 🎯 느낀 점

**네 개 중 세 개가 "디버깅용으로 넣었다가 안 지운 것"이거나 "일단 되게 하려고 건너뛴 것"이었음.**
시크릿 하드코딩, 평문 로깅, 업로드 인증 누락. 셋 다 [#3]({{< ref "/post/정담/3-관리자" >}})에서 하루에 1045줄 쏟아낼 때 생긴 것들임.

빨리 만드는 건 좋은데, **"나중에 고칠 것" 목록을 어딘가에 안 적어두면 나중이 두 달 뒤가 됨.**

그리고 4번 레이스 컨디션은 아무도 신고를 안 해줌. 에러가 안 나고 그냥 빈 화면이라서.
**조용히 실패하는 버그가 제일 오래 살아남는 듯.**

#정담 #보안 #JWT #레이스컨디션 #리팩터링
