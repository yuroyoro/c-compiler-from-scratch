[ステップ5：単項プラスと単項マイナス](https://www.sigbus.info/compilerbook/#%E3%82%B9%E3%83%86%E3%83%83%E3%83%975%E5%8D%98%E9%A0%85%E3%83%97%E3%83%A9%E3%82%B9%E3%81%A8%E5%8D%98%E9%A0%85%E3%83%9E%E3%82%A4%E3%83%8A%E3%82%B9)

除算の実行で `rdx` をゼロクリアしているので、除算の左辺が負数の場合の結果がおかしくなる

```
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rdi\n");
```

なんとなく数値を64bit inttegerとして扱っていて、除算の際に128bitに拡張する際の符号拡張を `cqo` で正しく実装する必要がある。また符号付き割り算の `idiv` を用いる

[パーサの変更](https://www.sigbus.info/compilerbook/#%E3%83%91%E3%83%BC%E3%82%B5%E3%81%AE%E5%A4%89%E6%9B%B4)


文法が以下のように、必ず `;` を必要とするようになったので、それまでvalidだった `42` や `3+4` はエラーになるようになった

```
program    = { stmt }
stmt       = expr ";"
```

以下のように、プログラムの終端も文の区切りとするように変更した
```
program    = { stmt }
stmt       = expr (";" | EOF)
```

