## [ステップ5：単項プラスと単項マイナス](https://www.sigbus.info/compilerbook/#%E3%82%B9%E3%83%86%E3%83%83%E3%83%975%E5%8D%98%E9%A0%85%E3%83%97%E3%83%A9%E3%82%B9%E3%81%A8%E5%8D%98%E9%A0%85%E3%83%9E%E3%82%A4%E3%83%8A%E3%82%B9)

除算の実行で `rdx` をゼロクリアしているので、除算の左辺が負数の場合の結果がおかしくなる

```
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rdi\n");
```

なんとなく数値を64bit inttegerとして扱っていて、除算の際に128bitに拡張する際の符号拡張を `cqo` で正しく実装する必要がある。また符号付き割り算の `idiv` を用いる

## [パーサの変更](https://www.sigbus.info/compilerbook/#%E3%83%91%E3%83%BC%E3%82%B5%E3%81%AE%E5%A4%89%E6%9B%B4)


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

## 関数呼び出し時のスタックポインタの16byteアライン

8ccではpush, popを生成するたびにスタックポインタをコンパイル時にカウントしているが、9ccではそのような処理が見当たらない。
9ccの方式が理解できてないので、まずはわかりやすく8ccのようにスタックポインタを計算することにした

が、ifなどの分岐が入ってくると、実行時にパスがかわるのでコンパイラで静的に事前のスタックポインタ計算ができなくなる…。
よって、以下2つの対策を考えた。

1. スタック操作に常に16byteアラインされるようにする。つまり、pushしたらさらに `sub rsp, 8` を吐き、pop時は `add rsp, 8` を吐く。つまりスタックを常に16byteづつ移動させる

2. 関数呼び出し前の prologueで動的にrspを計算する。rspを16で割って余りがでるならsub rsp, 8するようにする。

よくわからないのでgccで吐いたアセンブラを見てみる。
gccでは、関数を呼び出しを含む関数の場合にrspを `roundup(ローカル変数の個数) * 16` だけずらしていたので、これに習うことにする


## 16byteアラインする9ccのroundup関数

整数xをalign倍の数値に揃えるコードがこうなっていて、よく思いつくなーって感心しました。

(x + align - 1) & ~(align - 1);

例えばx=35で16に揃えようとすると、

= (35 + 16 - 1 = 50 = ) & ~ (15 = 00001111)
= (00110010) & (11110000)
= (00110000)
= 48

ただしalignが2^nの場合に限る


## 構文木の見直し

文と式を明確に区別するために、 `ND_STMT` と `ND_EXPR` を導入した。

```
  program    = func *
  func       = ident "("  (ident ",")* ")" block
  block      = "{" stmt* "}"
  stmt       = block
             | expr ";"
             | "return" expr ";"
             | "if" "(" expr ")" stmt [ "else" stmt ]
             | "while" "(" expr ")" stmt
             | "for" "(" expr? ";" expr? ";" expr? ")" stmt
  expr       = assign
  assign     = equality [ "=" assign ]
  equality   = relational ("==" relational | "!=" relational)*
  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
  add        = mul ("+" mul | "-" mul)*
  mul        = unary ("*" unary | "/" unary)*
  unary      = ("+" | "-")? term
  term       = num | ident | call | (" expr ")"
  call       = ident "(" (expr ",")* ")"
  ident      = A-Za-z0-9_
```
