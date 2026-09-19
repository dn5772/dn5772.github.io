---
title: "Tap Smith 개발 일지 #5 - 일지에서 빼먹은 것: Docker 배포랑 다국어"
description: "1~4편이 건너뛴 초반 작업. 멀티스테이지 빌드, nginx 리버스 프록시, i18n"
date: 2026-02-06T00:00:00Z
draft: false
tags: ["게임개발", "Tap Smith", "Docker", "nginx", "MySQL", "다국어", "배포"]
categories: ["game-dev/tap-smith"]
---

# Tap Smith 개발 일지 #5 - 일지에서 빼먹은 것

지난 네 편이 API → 인증 → DB 테스트 → 일일 미션 순서로 갔는데, 정작 **제일 먼저 만들어둔 것들**을 건너뛰었다.
1월 31일 ~ 2월 1일에 해둔 배포 구성이랑 다국어. 오늘 몰아서 정리한다.

## 왜 처음부터 Docker였나

로컬에 MySQL 깔기가 싫었다. 그게 다다.

Node 버전도 팀마다 다를 거고, MySQL 8.4를 로컬에 깔면 나중에 지우기도 귀찮다. 그래서 처음부터 `docker-compose up -d` 한 방으로 끝나게 만들어뒀다.

컨테이너 3개다.

```
nginx (80)  ──▶  app (3000, 내부만)  ──▶  db (MySQL 8.4)
```

## Dockerfile - 멀티스테이지 + 비루트

```dockerfile
# Build stage
FROM node:24-alpine AS builder
WORKDIR /app
COPY package*.json ./
RUN npm ci

# Runtime stage
FROM node:24-alpine
WORKDIR /app
RUN apk add --no-cache dumb-init
COPY --from=builder /app/node_modules ./node_modules
COPY package*.json ./
COPY src ./src

# 비루트 유저
RUN addgroup -g 1001 -S nodejs
RUN adduser -S nodejs -u 1001
USER nodejs

EXPOSE 3000
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD node -e "require('http').get('http://localhost:3000/health', (r) => {if (r.statusCode !== 200) throw new Error(r.statusCode)})"

ENTRYPOINT ["dumb-init", "--"]
CMD ["npm", "start"]
```

신경 쓴 거 세 가지.

**1. `dumb-init`.** Node를 PID 1로 띄우면 시그널 처리가 제대로 안 된다.
`docker-compose down` 할 때 SIGTERM이 안 먹어서 10초 기다렸다가 강제 종료되는 그 현상. `dumb-init` 을 ENTRYPOINT에 끼우면 깔끔하게 내려간다.

**2. 비루트 유저.** 컨테이너 안에서 root로 돌 이유가 없다. `nodejs` 유저 만들어서 그걸로 실행.

**3. HEALTHCHECK.** 이게 다음 항목이랑 연결된다.

## DB가 준비될 때까지 기다리게 하기

처음에 `depends_on: - db` 만 걸었더니 API가 계속 죽었다.
**`depends_on` 은 컨테이너가 "시작"되기만 기다리지, MySQL이 실제로 쿼리를 받을 준비가 됐는지는 안 본다.** MySQL은 첫 부팅에 초기화를 하느라 수십 초가 걸린다.

그래서 헬스체크를 걸고 `service_healthy` 를 조건으로 준다.

```yaml
app:
  depends_on:
    db:
      condition: service_healthy

db:
  healthcheck:
    test: ["CMD", "mysqladmin", "ping", "-h", "localhost", "-u", "root", "-p${DB_ROOT_PASSWORD:-rootpassword}"]
    interval: 10s
    timeout: 5s
    retries: 5
```

이거 하고 나서 `docker-compose up` 한 번에 깔끔하게 뜬다. [#3편]({{< ref "/post/Tap-Smith/개발-일지-3" >}})에서 "스키마 자동 생성도 깔끔하게 끝남"이라고 썼던 게 사실 이 구성 덕분이었다.

## nginx - 굳이 왜 끼웠나

API 하나 띄우는데 리버스 프록시까지 필요한가 싶었는데, 넣어두니까 편한 게 꽤 있었다.

```nginx
upstream api_backend {
    server app:3000;
    keepalive 32;
}

server {
    listen 80;

    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;

    client_max_body_size 10M;

    location / {
        proxy_pass http://api_backend;
        proxy_http_version 1.1;
        proxy_set_header Connection "";
        ...
    }

    location /health {
        proxy_pass http://api_backend/health;
        access_log off;
    }
}
```

- **app을 외부에 노출 안 한다.** `ports` 가 아니라 `expose` 라서 3000번은 컨테이너 네트워크 안에서만 열린다. 밖에서 닿는 건 nginx의 80뿐이다.
- **보안 헤더를 앱 코드에서 뺄 수 있다.** Fastify에서 매번 붙이는 것보다 여기 한 번 적는 게 낫다.
- **Gzip이랑 keepalive.** `proxy_http_version 1.1` + `Connection ""` 조합으로 업스트림 커넥션을 재사용한다.
- **`/health` 는 access_log 끈다.** 30초마다 찍히는 헬스체크 로그로 access.log가 도배되는 걸 막는다.

나중에 HTTPS 붙일 때도 앱은 안 건드리고 nginx만 고치면 된다.

## 프론트 다국어

게다 UI 문자열은 `front/locales/ko.json`, `en.json` 로 분리한다.
키는 화면 단위로 네임스페이스를 줬다.

```js
"tabs.market": "Market",
"tabs.mining": "Mining",
"tabs.smithy": "Smithy",
"mining.tapTitle": "Tap to Mine",
"mining.pickaxeShop": "Pickaxe Workshop",
"smithy.enchant": "Enchant",
"missions.checkin": "Daily Check-in",
"missions.checkinHint": "Once per day",
```

`i18n.js` 안에 **fallback 딕셔너리를 통째로 들고 있다.** 로케일 JSON을 못 불러와도 UI가 빈 칸으로 뜨지 않게 하려고.
Telegram Mini App이라 네트워크가 불안정한 상황이 충분히 있을 수 있어서 이렇게 둔다.

Telegram은 `initDataUnsafe.user.language_code` 로 유저 언어를 알려주니까, 첫 접속 때 그걸로 기본 언어를 정한다.

## 느낀 점

배포 구성은 만들 땐 지루한데 나중에 제일 고마운 부분인 듯.
[#3편]({{< ref "/post/Tap-Smith/개발-일지-3" >}})에서 DB 테스트 10개가 655ms만에 다 통과한 것도, 사실 그 전에 `docker-compose up` 한 방으로 MySQL이 스키마랑 더미 데이터까지 들고 뜨게 해둔 덕분이었다.

`depends_on` 이 "준비됨"을 보장 안 한다는 건 이번에 제대로 알았다. healthcheck랑 `service_healthy` 세트로 기억해둬야 할 듯.

다음엔 Telegram Mini App 실제 배포하면서 겪은 것들 써보겠다!
