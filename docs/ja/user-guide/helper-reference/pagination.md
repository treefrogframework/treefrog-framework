---
title: ページネーション
page_id: "080.080"
---

## ページネーション

１ページでは表示できないほどのデータがあるとき、これらを複数のページに分割して表示する機能のことを**ページネーション**、または**ページング**と呼び、Webアプリケーションでよく提供される機能の１つです。

TreeFrog Framework では、ごく基本的な機能をもつページネーションクラスを用意しています。TPaginator というクラスを使って、実装していきます。以下の例は ERB で記述しますが、Otama でも同様なものになります。

まず、アクションの中で、表示するページ番号をクエリ引数を使って受け取ります。その番号に該当するモデルのリストを取得し、ビューへ渡します。

```c++
int current = httpRequest().queryItemValue("page", "1").toInt();
int totalCount = Blog::count();

// 1ページに最大１０件の項目を表示。ページ番号は５個表示。
TPaginator pager(totalCount, 10, 5);
pager.setCurrentPage(current);  // 現在のページを設定
texport(pager);

// 該当する項目を取得し、ビューへ受け渡し
QList<Blog> blogList = Blog::getBlogs( pager.itemCountPerPage(), pager.offset() );
texport(blogList);
render();
```

次はビューです。<br>
ページ番号を表示するには、部分テンプレートを使用するのが良いでしょう。<br>
次の例では、渡された TPaginator オブジェクトを使って、ページ番号とそのリンクを描画しています。urlq()メソッドは、現在のアクションに対して、指定したクエリ引数を追加したURLを生成します。

テンプレート： views/partial/pagination.erb

```
<%#include <TPaginator> %>
<% tfetch(TPaginator, pager); %>
<div class="pagination">
  <%== linkToIf(pager.hasPrevious(), "Prev", urlq("page=" + QString::number( pager.previousPage() ))) %>

  <% for (QListIterator<int> i(pager.range()); i.hasNext(); ) {
      int page = i.next(); %>
      <%== linkToIf((page != pager.currentPage()), QString::number(page), urlq("page=" + QString::number(page))); %>
  <% } %>

  <%== linkToIf(pager.hasNext(), "Next", urlq("page=" + QString::number( pager.nextPage() ))) %>
</div>
```

部分テンプレートを描画する方法は、次のとおりでした。

```
<%== renderPartial("pagination") %>
```

また、このテンプレートでは、コントローラから渡されたモデルの一覧を描画します。繰り返しになるのでコードは省略します。[ジェネレータ](/ja/user-guide/generator/index.html){:target="_blank"}で作成した index テンプレートなどを参考にしてださい。

次に、モデルの取得です。<br>
該当するモデルの一覧を取得するには、データベースに対して、LIMIT 句 と OFFSET 句を指定したクエリを発行すれば良いでしょう。要件によっては、WHERE句やソートを指定する必要があるかもしれませんね。

このような SQL クエリはよく使われることから、次のTreeFrog Framework のユーティリティ関数を使うと短いコードで済みます。

```c++
QList<Blog> Blog::getBlogs(int limit, int offset)
{
    return tfGetModelListByCriteria<Blog, BlogObject>(TCriteria(), limit, offset);
}
```

※ [API リファレンス 参照](http://treefrogframework.org/tf_doxygen/tmodelutil_8h.html){:target="_blank"}


**追記：**<br>
ちなみにですが、ページネーションはそれほど難しい機能ではないので、TPaginator を使わなくとも独自に実装できると思います。これで足りなければ、実装にチャレンジしてみてください。