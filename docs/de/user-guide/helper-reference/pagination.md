---
title: Pagination
page_id: "080.080"
---

## Pagination

When data cannot be displayed on one single Web page, you might have already heard about **pagination** or **paging**. TreeFrog provides such a function which divides into multiple Web pages. This technique is very common for many web applications on the internet.

TreeFrog provides a class for pagination with very basic functions. In order to use TreeFrog's pagination ability you need to use the *TPaginator* class. The following example - written in ERB, but similar to Otama - will show you how to use pagination.

First of all, in the action of the *controller* class we will retrieve information about the current displaying page number by using query arguments. Then we get a list of models that correspond to that page number and pass them to the view.

```c++
int current = httpRequest().queryItemValue("page", "1").toInt();
int totalCount = Blog::count();

// Display up to ten items per page. The argument '5' specifies the number of pages
// to show ‘around’ the current page on a pagination bar, and should be an odd number
TPaginator pager(totalCount, 10, 5);
pager.setCurrentPage(current);  // Set the page number that is supposed to be displayed
texport(pager);

// Obtain the corresponding items and add them to the view
QList<Blog> blogList = Blog::getBlogs( pager.itemCountPerPage(), pager.offset() );
texport(blogList);
render();
```

Now, let's have a look at the view.<br>
It is a good technique to use a partial template when you want to display page numbers.<br>
In the following example, we used the passed *TPaginator* object to draw the page numbers and their respective links. For each link, the *urlq()* method creates the URL and the specified query arguments to the current action for you.

Using template: views/partial/pagination.erb

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

Rendering a partial template is rather simple. Use the following *renderPartial()*  method to achieve this:

```
<%== renderPartial("pagination") %>
```

The passing argument "pagination" is the name of the template you want to include. Furthermore, this template draws a list of models passed from the controller. For more details about rendering templates, please refer to the [generator](/en//user-guide/generator/index.html){:target="_blank"} which generates templates such as *index*, *show* etc.

Next is the acquisition of the model.<br>
To obtain a list of applicable models, you can issue a query with the LIMIT and OFFSET parameters from the database. Depending on your requirements, you may need to specify a WHERE or to sort the obtaining models.

Because such SQL queries are commonly used, the following TreeFrog Framework utility function requires not much code.

```c++
QList<Blog> Blog::getBlogs(int limit, int offset)
{
    return tfGetModelListByCriteria<Blog, BlogObject>(TCriteria(), limit, offset);
}
```

Please refer the [API Reference](http://treefrogframework.org/tf_doxygen/tmodelutil_8h.html){:target="_blank"} page for more details about the model utility.

**Note:**<br>
Since pagination is not that difficult, you can also try to challenge the implementation without using the TPaginator class at all.