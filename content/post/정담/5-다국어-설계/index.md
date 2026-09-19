---
title: "정담 개발 일지 #5 - 다국어를 어떻게 저장할지 먼저 정했음"
description: "필드 안에 {ko, en}을 넣을까 파일을 쪼갤까. 코드 치기 전에 설계 문서부터 씀"
slug: "jung-dam-5"
date: 2026-02-02T23:19:00+09:00
draft: false
tags: ["정담", "웹개발", "다국어", "i18n", "설계", "VercelBlob"]
categories: ["web/jung-dam"]
---

# 정담 개발 일지 #5 - 다국어를 어떻게 저장할지 먼저 정했음

외국인 손님이 꽤 온다고 해서 **영어 메뉴**를 붙이기로 했음.

근데 이번엔 바로 코드를 안 쳤음. [#4]({{< ref "/post/정담/4-저장소" >}})에서 하루에 세 번 갈아엎고 나니까, 저장 구조를 잘못 잡으면 얼마나 피곤한지 알게 됐음.
그래서 설계 문서를 먼저 썼음.

```
docs/menu-localization.md          ← 데이터 형식 설계
docs/menu-implementation-process.md ← Phase 1~6 작업 계획
```

## 🤔 두 가지 선택지

### 방법 A. 필드 안에 언어를 넣기

```json
{
  "title": { "ko": "간장마늘치킨", "en": "Garlic Soy Chicken" },
  "ingredients": { "ko": "닭, 마늘, 간장", "en": "Chicken, garlic, soy sauce" },
  "price": "$18"
}
```

파일 하나로 끝남. 관리자에서 양쪽 언어를 같이 편집하기도 편함.

**근데 손님한테 나가는 페이로드가 두 배임.** 한국어로 보는 손님이 영어 메뉴까지 통째로 받아감.

### 방법 B. 로케일별로 파일을 쪼개기

```
menuData-ko.json
menuData-en.json
```

```json
{ "title": "간장마늘치킨", "ingredients": "닭, 마늘, 간장", "price": "$18" }
```

손님은 자기 언어 파일 하나만 받음. 항목 구조도 단순해짐.

**대신 두 파일이 어긋날 수 있음.** 한쪽에만 항목이 추가되면 순서가 밀려서 엉뚱한 재료가 붙음.

## ✅ B로 감

**조회가 압도적으로 많고 수정은 거의 없는 화면**이라서임.

메뉴 조회는 손님이 올 때마다 일어나고, 메뉴 수정은 한 달에 몇 번임.
그럼 **조회 쪽을 가볍게 하고, 어긋날 위험은 쓰기 쪽에서 막는 게** 맞다고 봤음.

공통 필드는 두 파일에 똑같이 복제함.

| 필드 | 로케일별 | 공통 |
|---|---|---|
| `name` (카테고리) | ○ | |
| `description` | ○ | |
| `title` | ○ | |
| `ingredients` | ○ | |
| `price` | | ○ |
| `image` | | ○ |

`price` 랑 `image` 는 언어랑 상관없으니 그대로 복제. 중복이 생기지만 파일이 두 장뿐이라 감당할 만함.

## 🛡️ 어긋남을 막는 장치 3개

B의 유일한 약점이 데이터 불일치라서, 그걸 막는 걸 설계에 넣었음.

**1. 저장은 항상 양쪽 동시에.** API가 한쪽만 받는 걸 허용하지 않음.

```
POST /api/menu   ←  { en: {...}, ko: {...} }
```

한쪽만 고치는 경로를 아예 안 만들면 어긋날 수가 없음.

**2. 검증 스크립트.** 두 파일의 `categoryId`, 항목 개수, 순서, `image`, `price` 가 같은지 확인.

```js
enData.categories.forEach((enCat, idx) => {
  const koCat = koData.categories[idx];
  console.assert(enCat.id === koCat.id, `Category ID mismatch at ${idx}`);
  console.assert(enCat.items.length === koCat.items.length, `Item count mismatch in ${enCat.id}`);
  enCat.items.forEach((enItem, i) => {
    console.assert(enItem.image === koItem.image, `Image mismatch`);
    console.assert(enItem.price === koItem.price, `Price mismatch`);
  });
});
```

**3. 관리자 UI에선 합쳐서 다룸.** 화면에서는 `{ ko, en }` 객체로 들고 있다가 **저장할 때만** 두 로케일로 쪼갬.

```js
const convertToLocaleSpecific = (menuData) => ({
  en: { categories: menuData.categories.map(c => ({ ...getLocale(c, 'en') })) },
  ko: { categories: menuData.categories.map(c => ({ ...getLocale(c, 'ko') })) },
});
```

**저장 형식이랑 편집 형식을 분리한 게 핵심임.** 손님은 쪼개진 걸 받고, 관리자는 합쳐진 걸 편집함.

## 📋 Phase 1~6

작업 계획도 같이 적었음.

| Phase | 내용 | 예상 |
|---|---|---|
| 1 | 데이터 준비 및 마이그레이션 | 2-3시간 |
| 2 | API 수정 | 4-6시간 |
| 3 | 관리자 UI 수정 | 6-8시간 |
| 4 | 프론트엔드 표시 수정 | 2-3시간 |
| 5 | 통합 테스트 | 3-4시간 |
| 6 | 배포 및 모니터링 | 1-2시간 |
| | **합계** | **18-26시간** |

오늘 한 건 Phase 1임. 기존 데이터를 백업하고 `menuData-ko.json` 을 만들어서 번역을 채워 넣음.

```
add: Phase 1: 데이터 준비 및 마이그레이션
```

**기존 Blob 데이터를 먼저 `backup/` 에 내려받아 뒀음.** 마이그레이션 중에 날리면 메뉴판이 통째로 사라짐.

## ⚠️ 위험 요소를 미리 적어둠

설계 문서 끝에 이런 걸 적었음.

- **데이터 불일치** → 저장 시 검증, 정기 체크
- **번역 누락** → 한쪽만 입력됐을 때 경고
- **배포 중 장애** → Preview에서 검증, Blob 백업, `git revert` 준비
- **보안** → JWT 만료 시간, HTTPS 강제, 관리자 활동 로그

마지막 항목을 적어놓고 정작 [#3]({{< ref "/post/정담/3-관리자" >}})에서 만든 구멍은 못 봤음.
두 달 뒤에 발견함..

## 🎯 느낀 점

**저장 형식이랑 편집 형식이 같을 필요가 없다**는 게 이번에 제일 크게 배운 것.
화면에선 합쳐서 쓰고 저장은 쪼개서 하면, 편함이랑 성능을 둘 다 가져갈 수 있음.

그리고 문서를 먼저 쓰니까 "필드 안에 넣을까 파일 쪼갤까"를 코드로 시험해보지 않고 결정할 수 있었음.
[#4]({{< ref "/post/정담/4-저장소" >}})에서 두 시간 걸린 걸 오늘은 안 겪었음.

다음 글에서 Phase 2~6 구현.

#정담 #다국어 #i18n #설계 #VercelBlob
