---
title: /[Mac/]Neonim-설치
date: 2024-04-22
description: Neovim
tags: 
    - neovim
    - vim
    - install
    - setting
categories:
    - neovim
---

## [Mac]Neonim-설치

### 네오빔
자기전에 유튜브 보다가 neovim이라는 IDE를 알게됐다.\
내가 주로사용하는 IDE는 VScode와 Pychram인데 neovim이 너무 맘에 들어서 갈아타려고 마음 먹었다.

1학년 입학하자마자 C언어 보다 먼저 배운게 vi 사용법 이었기 때문에 vi 사용법도 어느정도 알고 무엇보다 간지가 나서 써보고 싶었다.
간지가 가장 큰 이유이지만, 다른 이유들도 적어보자면

1. 간지가 난다



### Neovim 설치

```zsh
brew install neovim
```
설치는 간단하다
```zsh
nvim
```
로 실행시켜볼 수 있다

### vim-plug 설치

```zsh
sh -c 'curl -fLo "${XDG_DATA_HOME:-$HOME/.local/share}"/nvim/site/autoload/plug.vim --create-dirs \
      https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim'
```
