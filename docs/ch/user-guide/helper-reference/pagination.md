---
title: 分页
page_id: "080.080"
---

## 分页

当数据不能在一个页面上显示的时候, 你可能已经想过*分页*. Treefrog提供了将数据分到多个页面的功能. 这是互联网许多网页应用的一个非常常用的技术.

Treefrog提供了有基础功能的分页类. 要使用Treefrog的分页功能, 你需要使用*TPaginater*类. 下面的例子- 用ERB写的, Otama也类似, 将展示如何使用分页.

首先, 在*控制器(controller)*类的操作(action)中我们将通过查询参数获取关于当前显示页的信息. 然后我们获取对应页面的模型(model)的列表, 然后传递给视图(view).

```c++
int current = httpRequest().queryItemValue("page", "1").toInt();
int totalCount = Blog::count();

//每页最多显示10项, 参数'5'指定显示的页
//要在分页条上当前页前后的页数, 应为奇数
TPaginator pager(totalCount, 10, 5);
pager.setCurrentPage(current);  // 设定要显示的texport(page)的页数;

//获得相应的项并将它们添加到视图(view)中
QList<Blog> blogList = Blog::getBlogs( pager.itemCountPerPage(), pager.offset() );
texport(blogList);
render();
```

现在, 让我们看看视图(view).<br>
当你想要显示页码时, 使用代码块(partial)模版是一个好方法.
下面的例子, 我们使用传递的*TPaginater*对象来描绘页码和它们对应的连接.*urlq()*方法为当前的操作(action)给每个链接创建了URL和指定的查询参数.

使用模版:views/partial/pagination.erb

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

渲染一个代码块(partial)模版是相当的简单. 使用下面的*renderPartial()*方法来实现:

```
<%== renderPartial("pagination") %>
```

传递的参数"pageination"是你想要包含的模版的名字. 还有, 这个模版绘制了从控制器(controller)传递过来的模型(model)列表. 关于渲染模版的更多详细信息, 请参考[生成器](/ch/user-guide/generator/index.html){:target="_blank"}, 生成器生成了*index*, *show*等模版.
接下来是获得模型(model).<br>
要获得应用模型(model)的清单, 可以在数据库中执行一个带有LIMIT和OFFSET参数的查询. 取决于你的要求, 你可以定义一个WHERE或者排序获得的模型(models).

因为这样的SQL查询经常使用, 下面的Treefrog框架工具不需要多少代码.

```c++
QList<Blog> Blog::getBlogs(int limit, int offset)
{
   return tfGetModelListByCriteria<Blog, BlogObject>(TCriteria(), limit, offset);
}
```

请参阅[API 参考](http://treefrogframework.org/tf_doxygen/tmodelutil_8h.html){:target="_blank"}获得更多关于模型(model)工具的信息.

**说明:**<br>
因为分页不是这么困难, 你也可以尝试完全不用Tpaginator类来实现.