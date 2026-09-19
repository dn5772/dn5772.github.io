---
title: "정담 개발 일지 #3 - 관리자 페이지를 하루에 다 만들었음"
description: "JWT 로그인, 메뉴 CRUD, 드래그 정렬. 커밋 하나에 1045줄"
slug: "jung-dam-3"
date: 2026-01-28T14:21:00+09:00
draft: false
tags: ["정담", "웹개발", "관리자", "JWT", "bcrypt", "DndKit"]
categories: ["web/jung-dam"]
---

# 정담 개발 일지 #3 - 관리자 페이지를 하루에 다 만들었음

메뉴가 바뀔 때마다 내가 JSON 고치고 배포하는 건 말이 안 된다.
**사장님이 직접 고칠 수 있어야 한다.** 그래서 관리자 페이지를 만들었다.

커밋 하나에 1045줄이 들어갔다.

```
src/app/admin/page.js             516 ++++++++++++++
src/app/admin/page.module.css     272 ++++++++
src/app/api/menu/route.js         138 ++++
src/app/api/auth/login/route.js    68 ++
src/app/api/upload/route.js        51 ++
5 files changed, 1045 insertions(+)
```

## 로그인은 비밀번호 하나로

쓰는 사람이 사장님 한 명이다. 회원가입도 없고 권한 구분도 없다.
그래서 **유저 테이블도 안 만들었다.**

```
POST /api/auth/login  →  { password }  →  { token }
```

- 비밀번호는 **bcrypt 해시**로 환경변수에 둠 (`ADMIN_PASSWORD_HASH`)
- 맞으면 JWT 발급, 이후 요청은 `Authorization: Bearer <token>`

유저가 한 명일 때 DB를 안 만드는 건 게으른 게 아니라 맞는 선택인 것 같다. 테이블 하나 만들면 마이그레이션이 따라오고 백업이 따라온다.

> 다만 이때 짠 로그인 라우트에 **꽤 심각한 문제 두 개**가 있었음.
> 두 달 뒤에 발견해서 [#7]({{< ref "/post/정담/7-보안" >}})에서 갈아엎음.

## 메뉴 CRUD API

라우트 하나에 다 몰아넣었다.

| 메서드 | 하는 일 |
|---|---|
| `GET /api/menu` | 메뉴 전체 조회 (인증 없음, 손님도 씀) |
| `POST /api/menu` | 메뉴 전체 덮어쓰기 |
| `PATCH /api/menu` | 항목 단위 추가/수정/삭제 |

`PATCH` 는 `action` 으로 분기한다.

```js
{ action: 'add',    categoryId, data }
{ action: 'update', categoryId, itemIndex, data }
{ action: 'delete', categoryId, itemIndex }
```

전체 저장(`POST`)이랑 항목 저장(`PATCH`)을 **둘 다** 만들어둔 게 나중에 문제가 된다.
관리자 화면에 저장 버튼이 두 개 생기는데, 사장님 입장에선 뭘 눌러야 하는지 알 수가 없다. 이건 [#8]({{< ref "/post/정담/8-모바일" >}})에서 정리한다.

## 이미지 업로드

```
POST /api/upload  →  파일  →  { url }
```

받아서 저장하고 URL을 돌려준다. 메뉴 항목엔 그 URL만 들어간다.

항목을 **삭제할 때 이미지도 같이 지우게** 해놨다. 안 그러면 안 쓰는 이미지가 계속 쌓이다.

```js
if (itemToDelete.image?.startsWith('https://')) {
  await del(itemToDelete.image, { token: BLOB_TOKEN });
}
```

> 이 업로드 엔드포인트에도 구멍이 있었는데, 역시 [#7]({{< ref "/post/정담/7-보안" >}})에서 다룸.

## 드래그로 메뉴 순서 바꾸기

메뉴판은 순서가 중요하다. 대표 메뉴가 맨 위에 있어야 한다.

`@dnd-kit` 을 쓴다. react-beautiful-dnd가 더 유명한데 유지보수가 멈췄고, DnD Kit은 **터치를 기본으로 지원**한다.
사장님이 폰으로 쓸 걸 생각하면 이게 결정적이었다.

## 느낀 점

하루에 1045줄은 좀 무리였다. 돌아가긴 하는데 **급하게 짠 흔적이 그대로 남는다.**
디버깅한다고 넣은 `console.log`, "일단 되게" 하려고 건너뛴 검증, 저장 버튼 두 개 같은 것들.

두 달 뒤에 그걸 전부 되갚게 된다..

그리고 아직 제일 큰 문제가 안 드러났다. **이 메뉴 데이터를 어디에 저장할 거냐** 하는 것.
로컬에선 완벽하게 잘 돌아갔는데, 오늘 저녁에 배포하고 나서 알게 된다.
