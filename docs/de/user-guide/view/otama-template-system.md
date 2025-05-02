---
title: Otama Template System
page_id: "070.020"
---

## Otama Template System

Otama is a template system that completely separates the presentation logic from the templates. It is a system made especially for the TreeFrog Framework.

Views are created for the Otama system when the configuration file (development.ini) is edited as the following and then the generator makes a scaffolding.

```
 TemplateSystem=Otama
```

The template is written in full HTML (with the .html file extension). A "mark" is used for the tag elements where logic code is to be inserted. The presentation logic file (.otm) is written with the associated C++ code, and the mark. This will be then automatically converted to C++ code when the shared view library is built.

<div class="img-center" markdown="1">

![View Convention]({{ site.baseurl }}/assets/images/documentation/views_conv.png "View Convention")

</div>

Basically, a set of presentation logic and template are made for every action. The file names are [action name].html and [action name].otm (case-sensitive). The files are placed in the "views/controller-name/" directory.

Once you have created a new template, in order for this to be reflected in the view shared library, you will need to run "make qmake" in the view directory

```
 $ cd views
 $ make qmake
```

If you have not already done so, it is recommended that you read the ERB chapter before continuing with this chapter, since there is much in common between the two template systems. Also, there is so much to learn about the Otama system that knowing ERB in advance will make it easier to understand.

## Output a String

We are going to the output of the statement "Hello world".<br>
On the template page, written in HTML , use a custom attribute called data-tf to put a "mark" for the element. The attribute name must start with "@". For example, we write as follows:

```
<p data-tf="@hello"></p>
```

We've used paragraph tags (\<p\> \</p\>) around the @hello mark.<br>
In the mark you may only use alphanumeric characters and the underscore '_'. Do not use anything else.

Next, we'll look at the presentation logic file in C++ code. We need to associate the C++ code with the mark made above. We write this as follows:

```
 @hello ~ eh("Hello world");
```

We then build, run the app, and then the view will output the following results:

```
<p>Hello world</p>
```

The tilde (~) that connects the C++ code with the mark that was intended for the presentation logic means effectively "substitute the contents of the right side for the content of the element marked". We remember that the eh() method outputs the value that is passed.

In other words, the content between the p tags (blank in this case) is replaced with "Hello world". The data-tf attribute will then disappear entirely.

In addition, as an alternative way of outputting the same result, it's also possible to write as follows:

```
 @hello ~= "Hello world"
```

As in ERB; The combination of ~ and eh() method can be rewritten as '~='; similarly, the combination of ~ and echo() method can be rewritten as '~=='.

Although we've output a static string here, in order to simplify the explanation, it's also possible to output a variable in the same way. Of course, it is also possible to output an object that is passed from the controller.

##### In brief: Place a mark where you want to output a variable. Then connect the mark to the code.

## Otama Operator

The symbols that are sandwiched between the C++ code and the mark are called the Otama operator.

Associate C++ code and elements using the Otama operator, and then decide how these should function. In the presentation logic, note that there must be a space on each side of the Otama operator.

This time, we'll use a different Otama operator. Let's assume that presentation logic is written as the following (colon).

```
 @hello : eh("Hello world");
```

The Result of View is as follows:

```
 Hello world
```

The p tag has been removed. This is because the colon has the effect of "replace the whole element marked", with this result. Similar to the above, this could also be written as follows:

```
 @hello := "Hello world"
```

## Using an Object Passed from the Controller

In order to display the export object passed from the controller, as with ERB, you can use it after fetching by tfetch() macro or T_FETCH() macro. When msg can export an object of QString type, you can describe as follows:

```
 @hello : tfetch(QString, msg);  eh(msg);
```

As with ERB, objects fetched are defined as a local variable.

Typically, C++ code will not fit in one instruction line. To write a C++ code of multiple rows for one mark, write side by side as normal but put a blank line at the end. The blank line is considered to be one set of the parts of the mark. Thus, between one mark and the next a blank line (including a line with only blank characters) acts as a separator in the presentation logic.

##### In brief: logic is delimited by an empty line.

Next, we look at the case of wanting to display an export object in two different locations. In this case, if you describe it at #init, it will be called first (fetched). After that, it can be used freely in the presentation logic. It should look similar to the following:

```
 #init : tfetch(QString, msg);

 @foo1 := msg

 @foo2 ~= QString("message is ") + msg
```

With that said, for exporting objects that are referenced more than once, use the fetch processing at *#init*.

Here is yet another way to export output objects.<br>
Place "$" after the Otama operator. For example, you could write the following to export the output object called *obj1*.

```
 @foo1 :=$ obj1
```

This is, output the value using the eh() method while fetch() processing for obj1. However, this process is only an equivalent to fetch processing, the local variable is not actually defined.

To obtain output using the echo() method, you can write as follows:

```
 @foo1 :==$ obj1
```

Just like ERB.

##### In brief: for export objects, output using =$ or ~=$.

## Loop

Next, I will explain how to use loop processing for repeatedly displaying the numbers in a list.<br>
In the template, we want a text description.

```
<tr data-tf="@foreach">
  <td data-tf="@id"></td>
  <td data-tf="@title"></td>
  <td data-tf="@body"></td>
</tr>
```

That is exported as an object in the list of Blog class named blogList. We want to write a loop using a for statement. The while statement will also be similar.

```
 @foreach :
 tfetch(QList<Blog>, blogList);    /* Fetch processing */
 for (auto &b, blogList) {
     %%
 }

 @id ~= b.id()

 @title ~= b.title()

 @body ~= b.body()
```
　
The %% sign is important, because it refers to the entire element (*@foreach*) of the mark. In other words, in this case, it refers to the element from \<tr\> up to \</ tr\>. Therefore, by repeating the \<tr\> tags, the foreach statement which sets the value of each content element with *@id*, *@title*, and *@body*, results in the view output being something like the following:

```
<tr>
  <td>100</td>
  <td>Hello</td>
  <td>Hello world!</td>
</tr><tr>
  <td>101</td>
  <td>Good morning</td>
  <td>This morning ...</td>
</tr><tr>
   :    (← Repeat the partial number of the list)
```

The data-tf attribute will disappear, the same as before.

## Adding an Attribute

Let's use the Otama operator to add an attribute to the element.<br>
Suppose you have marked such as the following in the template:

```
<span data-tf="@spancolor">Message</span>
```

Now, suppose you wrote the following in the presentation logic:

```
 @spancolor + echo("class=\"c1\" title=\"foo\"");
```

As a result, the following is output:

```
 <span class="c1" title="foo">Message</span>
```

In this way, by using the + operator, you can add only the attribute.<br>
As a side note, you cannot use the eh() method instead of the echo() method, because this will take on a different meaning when the double quotes are escaped.

Another method that we could also use would be written as follows in the presentation logic:

```
 @spancolor +== "class=\"c1\" title=\"foo\""
```

echo() method can be rewritten to '=='.

In addition, for the same output result, the following alternative method could be also written like:

```
 @spancolor +== a("class", "c1") | a("title", "foo")
```

The a() method creates a THtmlAttribute object that represents the HTML attribute, using \| (vertical bar) to concatenate these. It is not an THtmlAttribute object after concatenation but, if you output with the echo() method, they are converted to a string of *key1="val1", key2="val2"…*, means that attributes are added as a result.

You may use more if you wish.

## Rewriting the \<a\> Tag

The \<a\> tag can be rewritten using the colon ':' operator. It acts as described above.<br>
To recap a little; the \<a\> tag is to be marked on the template as follows:

```
<a class="c1" data-tf="@foo">Back</a>
```

As an example; we can write the presentation logic of the view (of the Blog) as follows:

```
 @foo :== linkTo("Back", urla("index"))
```

As a result, the view outputs the following:

```
<a href="/Blog/index/">Back</a>
```

Since the linkTo() method generates the \<a\> tag, we can get this result. Unfortunately, the class attribute that was originally located has disappeared. The reason is that this operator has the effect of replacing the whole element.

If you want to set the attribute you can add it as an argument to the linkTo() method:

```
 @foo :== linkTo("Back", urla("index"), Tf::Get, "", a("class", "c1"))
```

The class attribute will also be output as a result like the same as above.

Although attribute information could be output, you wouldn't really want to bother to write such information in the presentation logic.<br>
As a solution there is the \|== operator. This has the effect of merging the contents while leaving the information of the attributes attached to the tag.

So, let's rewrite the presentation logic as follows:

```
 @foo  |== linkTo("Back", urla("index"))
```

As a result, the view outputs the following:

```
<a class="c1" href="/Blog/index/">Back</a>
```

The class attribute that existed originally remains; it does NOT disappear.

The \|== operator has a condition to merge the elements. That is the elements must be the same tags. In addition, if the same attribute is present in both, the value of the presentation logic takes precedence.

By using this operator, the information for the design (HTML attributes) can be transferred to the template side.

##### In brief: Leave the attribute related to the design of the template and merge it by using the \|== operator.

**Note:**<br>
The \|== operator is only available in this format (i.e. \|== ), neither '\|' on its own, nor '\|=' will work.

## Form Tag

Do not use the form tag \<form\> to POST data unless you have enabled the CSRF measures. It does not accept POST data but only describes the form tag in the template. We need to embed the secret information as a hidden parameter.

We use the \<form\> tag in the template. After putting the mark to the \<form\> tag of the template, merge it with the content of what the formTag() method is outputting

Template:

```
  :
<form method="post" data-tf="@form">
  :
```

Presentation logic:

```
 @form |== formTag( ... )
```

You'll be able to POST the data normally.

For those who have enabled CSRF measures and want to have more details about security, please check out the chapter [security]({{ site.baseurl }}/en/user-guide/security/index.html){:target="_blank"}.

## Erasing the Element

If you mark *@dummy* elements in the template, it is not output as a view. Suppose you wrote the following to the template.

```
<div>
  <p>Hello</p>
  <p data-tf="@dummy">message ..</p>
</div>
```
Then, the view will make the following results.

```
<div>
  <p>Hello</p>
</div>
```

## Erasing the Tag

You can keep a content and erase a start-tag and an end-tag only.<br>
For example, when using a layout, the \<html> tag is outputted by the layout file side, so you don't need to output it anymore on the template side, but leave the \<html> tag on the template side if you want your layout have based on HTML. <br>
Suppose you wrote the following to the template.

```
<html data-tf="@dummytag">
  <p>Hello</p>
</html>
```

Then, the view will make the following results.

```
<p>Hello</p>
```

You use these when you want to keep it in the Web design, but erase it from the view.

## Including the Header File

We talked about the presentation logic template being converted to C++ code. The header and user-defined files will not be included automatically and you must write them by yourself. However, basic TreeFrog header files can be included.

For example, if you want to include *user.h* and *blog.h* files, you would write these in at the top of the presentation logic.

```
 #include "blog.h"
 #include "user.h"
```

All the same as the C++ code!<br>
Lines beginning with an #include string are moved directly to the code view.

## Otama Operator

The following table describes the Otama operator which we've been discussing.

<div class="table-div" markdown="1">

| Operator | Description                                                                                                                                                                                | Remarks                                                |
|----------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------|
| :        | **Element replacement**<br>The components and subcomponents that are markedwill be completely replaced and output by the string inside the eh() or echo() method which are on the right-hand side of this operator. | %% means the elements themselves that can be replaced.
| ~        | **Content replacement**<br>The content of marked elementswill be replaced and output by the string inside the eh() or echo() method which are on the right-hand side of this operator.                    |                                                        |
| +        | **Attribute addition**<br>If you want to add an attribute to an element that is marked, use this operator plus a string inside the echo() method, which is supposed to be output on the right-hand side of this operator.                      | += is HTML escaping, perhaps not used that much.            |
| \|==     | **Element merger**<br>Based on the marked elements, the specified strings will be merged on the right-hand side of this operator.                                                                                  | '\|' and '\|=' are disabled.                           |

 </div><br>

Extended versions of these four operators are as follows.
With the echo() statement and eh() statement no longer being needed, you'll be able to write shorter code.

<div class="table-div" markdown="1">

| Operator                       | Description                                                                                                                                                                         |
|--------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| :=<br> :==<br> :=$<br> :==$    | Element replaced by an HTML escaped variable.<br> Element replaced by a variable.<br> Element replaced by an HTML escaped export object.<br> Element replaced by an export object.  |
| ~=<br>  ~==<br> ~=$<br> ~==$   | Content replaced by an HTML escaped variable.<br> Content replaced by a variable.<br>  Content replaced by an HTML escaped export object.<br> Content replaced by an export object. |
| +=<br>  +==<br>  +=$<br>  +==$ | Add an HTML escaped variable to an attribute.<br> Add a variable to an attribute.<br>  Add an HTML escaped export object to an attribute.<br> Add an export object to an attribute. |
| \|==$                          | Element merged with an export object.                                                                                                                                               |

</div><br>

## Comment

Please write in the form of  /*.. */,  if you want to write a comment in the presentation logic.

```
 @foo ~= bar    /*  This is a comment */
```

**Note:** In C++ the format used is "// .." but this can NOT be used in the presentation logic.