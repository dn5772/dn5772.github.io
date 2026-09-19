---
title: "ABIDE 개발 일지 #5 - 어드민 대시보드를 한 시간 반에 갈아엎음"
description: "Next.js 16 관리 페이지 8커밋. 권한 게이트, 무드 구절, 스케줄, 캠페인"
slug: "abide-5"
date: 2026-03-21T19:48:00+09:00
draft: false
tags: ["ABIDE", "NextJS", "React", "어드민", "JWT", "TanStackQuery"]
categories: ["app/abide"]
---

# ABIDE 개발 일지 #5 - 어드민 대시보드를 한 시간 반에 갈아엎음

[#4]({{< ref "/post/abide/4-인증" >}})에서 유저가 생겼으니 **운영할 화면**이 필요해졌음.

3월 21일 18:54~19:46, **1시간 52분에 커밋 8개.** 루트 레포 커밋 메시지가 이럼.

```
chore: update submodule pointers — admin panel full overhaul
```

## 🧱 스택

```
Next.js 16 (App Router) + React 19
TanStack Query · Tailwind v4 · recharts · jwt-decode
```

[정담]({{< ref "/post/정담/1-시작" >}})도 Next.js였는데, 여긴 **데이터 조회가 전부**라 TanStack Query를 얹었음.
어드민은 화면 열 때마다 최신 값을 봐야 하는데, 직접 `useEffect` + `fetch` 로 짜면 캐시·재검증·로딩 상태를 매 화면마다 다시 씀.

## 🔐 권한을 두 겹으로

```
18:54  feat: add AuthContext and install jwt-decode, recharts
18:55  feat: add auth guard to layout, update sidebar (remove feeds, add role-gated admin accounts)
19:37  feat: add admin account management page (super_admin only)
```

[#2]({{< ref "/post/abide/2-백엔드" >}})의 Admin API가 JWT를 주는데, 거기에 **역할**이 들어 있음.

같은 날 Admin 서버에도 이게 들어갔음.

```
feat: add name claim to JWT, add admin user management endpoints (super_admin only)
```

프론트에서는 `jwt-decode` 로 역할을 읽어서 **사이드바 메뉴 자체를 감춤.**
`super_admin` 이 아니면 어드민 계정 관리 메뉴가 안 보임.

근데 메뉴를 감추는 건 **UX일 뿐 보안이 아님.** URL을 직접 치면 들어감.
그래서 실제 차단은 서버에서 함. 프론트는 "보여줄지"만 정하고, 서버가 "줄지"를 정함.

레이아웃에 auth guard를 걸어서 토큰이 없으면 아예 진입을 막았음.

## 🐛 19:00 — 한 커밋에 버그 네 개

```
fix: delete feeds page, fix dashboard field names + add mood chart,
     fix display_name, add admin_note modal to reports
```

어드민 페이지를 실제로 열어보니 **API 응답 필드명이랑 화면이 안 맞았음.**

- 대시보드가 없는 필드를 읽어서 전부 `undefined`
- `display_name` 을 다른 이름으로 찾고 있었음
- 피드 페이지는 사이드바에서는 뺐는데 **파일이 남아 있었음**
- 신고 처리에 관리자 메모를 남길 데가 없었음

**백엔드랑 프론트를 따로 만들면 필드명이 거의 항상 어긋남.** 타입을 공유 안 하니까 실행해봐야 앎.

## 📅 19:06~19:35 — 운영 화면 세 개

```
19:06  feat: add mood verses management page
19:12  feat: add schedules management page (daily + weekly tabs)
19:35  feat: add campaigns management page
```

**무드 구절** — [#1]({{< ref "/post/abide/1-앱-뼈대" >}})의 Compass 화면이 "오늘 기분"으로 구절을 추천함.
그 매핑을 코드에 박아두면 바꿀 때마다 앱을 새로 내야 함. **어드민에서 고칠 수 있어야 함.**

**스케줄** — 일별/주별 탭. 매일 보낼 묵상 주제를 미리 꽂아둠.

**캠페인** — 특정 기간 이벤트.

셋 다 공통점이 있음. **콘텐츠를 코드에서 빼서 DB로 옮긴 것.**
[정담]({{< ref "/post/정담/3-관리자" >}})에서 메뉴판을 관리자 페이지로 뺀 거랑 정확히 같은 이유임 — 내가 매번 배포하면 안 됨.

## 📉 19:46 — 마지막 커밋

```
fix: remove explicit type annotation on recharts Tooltip formatter
```

recharts 타입이 라이브러리 내부 제네릭이랑 안 맞아서 **명시적 타입 주석을 뺐더니** 추론이 오히려 맞음.

TypeScript에서 가끔 있는 일인데, 타입을 직접 적는 게 항상 나은 게 아님. **라이브러리가 이미 잘 추론하는 자리는 안 건드리는 게** 나음.

## 🎯 느낀 점

**어드민은 빨리 만들수록 이득임.** 여기 쓴 두 시간이 나중에 "구절 하나 바꾸려고 앱 배포"하는 걸 통째로 없앰.

그리고 어드민을 사용자 서비스랑 **완전히 분리한 게** 맞았음([#2]({{< ref "/post/abide/2-백엔드" >}})).
1시간 52분에 8커밋을 밀어넣을 수 있었던 건, 여기가 깨져도 앱은 안 죽는다는 걸 알고 있어서임.

그다음 이틀은 성경 본문 쪽을 손봄.

#ABIDE #NextJS #React #어드민 #JWT
