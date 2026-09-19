---
title: "ABIDE 개발 일지 #2 - 서버를 네 개로 쪼갰음"
description: "Rust 코어 + Python AI + Admin API + Next.js 대시보드. 서브모듈 모노레포로 묶기"
slug: "abide-2"
date: 2026-03-13T23:05:00+09:00
draft: false
tags: ["ABIDE", "Rust", "Axum", "FastAPI", "모노레포", "Docker", "PostgreSQL"]
categories: ["app/abide"]
---

# ABIDE 개발 일지 #2 - 서버를 네 개로 쪼갰음

[#1]({{< ref "/post/abide/1-앱-뼈대" >}})에서 앱은 만들었는데 부를 서버가 없었음.
3월 13일 저녁에 백엔드를 통째로 올렸음.

**서버를 하나로 안 만들고 네 개로 쪼갰음.**

```
Flutter  →  Core API (Rust/Axum :8080)  →  AI Agent (Python/FastAPI :8000)
                                        ↘  Admin API (Python/FastAPI :8001)
                    ↕                             ↕
          PostgreSQL 16 + pgvector          Redis (세션/캐시)
```

## 🤔 왜 언어를 갈랐나

한 언어로 통일하면 편한데 일부러 안 했음. **요구사항이 정반대**라서임.

**Core (Rust/Axum)** — 인증, 성경 데이터, 유저 관리, 쿼터.
모든 요청이 여길 통과함. 지연이 그대로 체감되고, 24시간 떠 있어야 함. 로직은 단순한데 양이 많음.

**AI (Python/FastAPI)** — LangGraph 멀티 에이전트, Gemini 호출, pgvector RAG.
여긴 **생태계가 전부 Python**임. LangGraph를 Rust로 다시 짤 이유가 없음.

[seam]({{< ref "/post/seam/4-seam-code" >}}) 때랑 똑같은 결론임. 런타임은 빠른 언어로, AI는 생태계 언어로.

**Admin API (Python/FastAPI)** — 어드민 전용. 사용자 서비스랑 **완전히 분리**함.
어드민 코드에 버그가 나도 앱은 멀쩡해야 하고, 권한 모델이 아예 다름.

**Admin Web (Next.js 16)** — 대시보드. [#5]({{< ref "/post/abide/5-어드민" >}})에서 크게 갈아엎음.

## 🔑 Core가 AI를 프록시함

Flutter가 AI 서버를 **직접 안 부름.** 항상 Core를 거침.

```
Flutter → Core (JWT 검증 + 쿼터 차감) → AI 서버
```

이렇게 한 이유가 세 개임.

1. **인증을 한 군데서만.** AI 서버가 JWT를 몰라도 됨
2. **쿼터를 여기서 깎음.** AI 호출은 돈이 드니까 Core가 관문이 돼야 함
3. **AI 서버를 외부에 안 엶.** 내부 네트워크에만 둠

그래서 서버 간 통신에 **내부 API 키**를 붙였음.

```
feat: 개선된 Internal API Key 검증 로직 및 공개 경로 처리 추가
feat: 예외 처리 및 로깅 개선: 내부 API 키 미들웨어와 전역 예외 처리기 추가
```

미들웨어로 걸되 헬스체크 같은 **공개 경로는 예외**로 빼야 함. 안 그러면 로드밸런서가 서버를 죽은 걸로 봄.

## 🧩 서브모듈 모노레포

레포를 하나로 할지 여러 개로 할지 고민하다가 **중간을 골랐음.**

```
the_seeker/              ← 오케스트레이션 레포
├── apps/abide/          ← 서브모듈
├── apps/admin-web/      ← 서브모듈
├── servers/core/        ← 서브모듈
├── servers/ai/          ← 서브모듈
├── servers/admin/       ← 서브모듈
└── infra/               ← 서브모듈
```

각 컴포넌트가 자기 git 레포고, 루트가 포인터만 들고 있음.

**장점**: 컴포넌트별로 배포·버전이 독립적임. Flutter 앱 고치는데 Rust 서버 히스토리가 안 섞임.
**단점**: 서브모듈 포인터 관리가 귀찮음.

그 귀찮음이 커밋 로그에 그대로 드러남.

```
chore: Sync all submodules to remote repositories
chore: Sync all submodules to remote repositories
chore: Sync all submodules to remote repositories
chore: Sync all submodules to remote repositories
```

루트 37커밋 중 **15개가 이 문장**임. 그래서 스크립트를 만들었음.

```bash
./push_submodules.sh    # 전부 커밋·푸시
./pull_submodules.sh    # EC2에서 전부 받아오기
```

14일에 `push_submodules.sh` 를 한 번 고쳤는데, 원래는 **자동으로 커밋까지** 했었음.

```
chore: Simplify push script to only push existing commits without auto-committing
```

의도치 않은 게 딸려 올라가서 **푸시만 하게** 바꿨음. 편의 스크립트가 커밋 메시지를 대신 써주면 나중에 히스토리를 못 읽음.

## 🗄️ DB는 PostgreSQL + pgvector

RAG 때문에 벡터 DB가 필요한데, **별도 벡터 DB를 안 붙였음.**
`pgvector` 확장이면 같은 Postgres에서 됨. 운영할 인프라가 하나 줄어듦.

Redis는 세션이랑 캐시용.

마이그레이션은 번호 붙인 SQL 파일을 순서대로.

```
infra/migrations/001_initial.sql
...
infra/migrations/009_*.sql
```

여기서 한 번 데였음. Docker의 `docker-entrypoint-initdb.d` 는 **DB가 처음 만들어질 때만** 돌아감.
이미 떠 있는 DB에 새 마이그레이션을 넣으려면 수동으로 돌려야 함. 모르고 `docker compose up` 만 다시 하면 아무 일도 안 일어남.

```bash
./deploy.sh migrate        # 신규 마이그레이션 적용
./deploy.sh migrate 009    # 특정 번호만
```

CLAUDE.md에 굵게 적어뒀음.

## 🚢 배포 설정에서 헤맸음

13일 저녁 커밋을 보면 이게 보임.

```
22:35  docs: Update DB, Redis, and internal service URLs to localhost for local deployment
22:38  docs: Change internal server communication URLs to public HTTPS domains
```

**3분 만에 반대로 뒤집음.**

내부 서비스끼리 `localhost` 로 부를지 공개 도메인으로 부를지인데, Docker Compose 안에서는 서비스명으로 붙고 EC2에서는 또 다름.
"로컬 개발"이랑 "컨테이너 안"이랑 "서버 배포"가 각각 다른 주소를 원하는데, 그걸 문서 한 장에 적으려니까 계속 꼬였음.

결국 환경변수로 빼고 `.env.example` 을 정리하는 걸로 갔음.

## 🎯 느낀 점

**서버를 네 개로 쪼갠 건 맞았고, 서브모듈은 반쯤 맞았음.**

언어를 나눈 건 확실히 이득이었음. Rust로 AI를 짰으면 LangGraph 생태계를 통째로 포기했을 것이고, Python으로 Core를 짰으면 상시 떠 있는 API 서버 비용이 올라갔을 것.

서브모듈은 **독립 배포는 얻었는데 포인터 관리 비용을 냄.** 루트 커밋의 40%가 동기화 커밋인 걸 보면, 혼자 하는 프로젝트엔 과했을 수도 있음.

다음은 이 앱의 본체임. AI 묵상 에이전트.

#ABIDE #Rust #Axum #FastAPI #모노레포 #PostgreSQL
