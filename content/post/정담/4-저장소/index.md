---
title: "정담 개발 일지 #4 - 저장소를 하루에 세 번 갈아엎었음"
description: "파일 → 메모리 → 파일 → Vercel Blob. 서버리스에선 파일에 못 쓴다는 걸 몸으로 배운 날"
slug: "jung-dam-4"
date: 2026-01-28T18:39:00+09:00
draft: false
tags: ["정담", "웹개발", "VercelBlob", "서버리스", "배포", "낙관적업데이트"]
categories: ["web/jung-dam"]
---

# 정담 개발 일지 #4 - 저장소를 하루에 세 번 갈아엎었음

[#3]({{< ref "/post/정담/3-관리자" >}})에서 관리자 페이지 만들고 배포했다.
그리고 저녁 내내 저장소를 세 번 바꿨다. 커밋 시간까지 붙여서 남겨둔다.

```
16:57  Fix Vercel deployment: Change menu API to memory-based storage
16:59  refactor: removing in-memory storage and using direct file access
18:10  feat: integrate Vercel Blob for menu data storage and image uploads
```

**16:57 → 16:59, 2분.** 이 2분이 이 글의 하이라이트다.

## 1차 - 파일에 쓰기 (로컬에선 완벽)

처음엔 `src/data/menuData.json` 을 `fs` 로 직접 읽고 쓴다.

```js
const data = JSON.parse(fs.readFileSync(MENU_PATH, 'utf8'));
// ...수정...
fs.writeFileSync(MENU_PATH, JSON.stringify(data, null, 2));
```

로컬에서 완벽하게 잘 된다. 관리자에서 메뉴 고치면 바로 반영되고, 새로고침해도 남아 있다.

배포하니까 **저장 버튼을 눌러도 아무 일이 안 일어난다.**
에러도 안 뜬다. 성공 응답이 오는데 새로고침하면 그대로다.

**Vercel 서버리스 함수의 파일시스템은 읽기 전용이다.** (`/tmp` 빼고)
머리로는 알고 있었는데 내 코드가 그걸로 죽는 걸 본 건 처음이었다.

## 2차 - 메모리에 올리기 (2분 만에 철회)

그래서 프로세스 메모리에 올려두고 거기서 읽고 쓰게 바꾼다.

```js
let menuCache = null;
if (!menuCache) menuCache = loadInitialData();
```

배포하니까 **저장은 된다.** 오 되네 하고 좋아한다.

그리고 좀 있다가 새로고침하니까 원래대로 돌아가 있다.

당연한 일이었다. 람다는 요청 없으면 죽고, 죽으면 메모리가 날아간다. 인스턴스가 여러 개면 저장한 인스턴스랑 읽는 인스턴스가 다를 수도 있다.
**"서버가 계속 살아 있다"를 전제로 한 설계였다.** 서버리스에선 그 전제가 틀린다.

2분 만에 되돌렸다.

```
75eec6d  +44  −17   메모리 저장소 도입
a8fee7f  +17  −76   메모리 저장소 제거
```

## 3차 - Vercel Blob (한 시간 걸림)

결국 메뉴 데이터를 **Blob에 JSON 파일로** 올린다.

```
menuData.json     ← 나중에 ko/en 으로 쪼개짐
```

DB를 안 쓴 이유는 이런다.

- 메뉴 항목이 수십 개다. 테이블 만들 규모가 아니다
- 하루에 몇 번 바뀔까 말까다. 동시성 걱정이 없다
- **이미지는 어차피 Blob에 올려야 한다.** 저장소를 둘로 나눌 이유가 없다
- 스키마 마이그레이션이 없다

`GET` 은 Blob에서 파일을 찾아 URL로 받아오고, `POST`/`PATCH` 는 수정해서 다시 올린다.

```js
const { blobs } = await list({ token: BLOB_TOKEN });
const menuBlob = blobs.find(b => b.pathname === 'menuData.json');
const data = await (await fetch(menuBlob.url)).json();
```

> 이때 짠 저장 로직(`list → del → put`)에 **레이스 컨디션**이 있었는데,
> 두 달 뒤에야 발견함. [#7]({{< ref "/post/정담/7-보안" >}})에서 다룸.

## 덤으로 낙관적 업데이트

Blob 왕복이 로컬 파일보다 확실히 느리다. 메뉴 하나 고칠 때마다 스피너가 돌면 답답한다.

그래서 **화면을 먼저 바꾸고 요청을 뒤에 보내게** 한다. 실패하면 되돌리고 에러를 띄운다.

```
클릭 → 화면 즉시 반영 → 요청 전송 → 실패 시 롤백 + 에러
```

메뉴판 편집은 실패할 일이 거의 없는 작업이다. 성공을 기본으로 깔고 가는 게 맞다.

## 그리고 README

```
docs: update README.md with project features, setup instructions, and API endpoints
```

오늘 하도 갈아엎어서, 지금 구조를 안 적어두면 일주일 뒤에 나도 까먹을 것 같았다.

## 느낀 점

**"로컬에서 되는데요"가 제일 무서운 말이다.** 파일 쓰기는 로컬에서 100% 돌아간다. 배포 환경이 다르다는 걸 코드가 알려주지 않는다.

그리고 2차(메모리)는 **1차랑 똑같은 착각을 다른 방식으로 반복한 것**이었다.
"이 프로세스가 계속 살아 있다"를 전제했다는 점에서 파일이나 메모리나 같다. 진짜 고쳐야 했던 건 저장 위치가 아니라 그 전제였다.

2분 만에 철회한 커밋이 남아 있는 게 좀 민망한데, 그래도 지우진 않을 거다. 이게 그날 실제로 있었던 일이니까.
