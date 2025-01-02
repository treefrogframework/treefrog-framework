---
title: Validation
page_id: "080.030"
---

## Validation

Sometimes, data that is sent as a request may not have the format that the developer has specified. For example, some users might put letters where numbers are required. Even if you have implemented Javascript to validate on the client side, tampering with the content of a request is not complicated, so a server side system for validating content is essential.

As mentioned in the [controller chapter]({{ site.baseurl }}/en/user-guide/controller/index.html){:target="_blank"}, data in received requests is expressed as hash format. Usually before sending the request data to the model, each value should be validated the value's format.

First, we will generate a validation class skeleton for validating request data (hash) for *blog*. Navigate to the application root directory and then execute the following commands.

```
 $ tspawn validator blog
   created   helpers/blogvalidator.h
   created   helpers/blogvalidator.cpp
   updated   helpers/helpers.pro
```

The validation rules are set in the construction of the generated BlogValidator class. The following example shows the implementation of a the rule for the title variable which must be made of at least 4 or more letters and must not have more than 20.

```c++
BlogValidator::BlogValidator() : TFormValidator()
{
   setRule("title", Tf::MinLength, 4);
   setRule("title", Tf::MaxLength, 20);
}
```

The enum value is the second argument. You can specify mandatory input, maximum/minimum string length, integer of maximum/minimum value, date format, e-mail address format, user-defined rules (regular expression), and so on (defined in tfnamespace.h).

There is also a fourth argument: setRule(). This sets the validation error message. If you don't specify a message (what we have done here), the message defined in *config/validation.ini* file is used.

Rules are implicitly set for "mandatory input". If you do NOT want an input to be "mandatory", describe the validation rule as follows:

```c++
setRule("title", Tf::Required, false);
```

<div class="center aligned" markdown="1">

**Rules**

</div>

<div class="table-div" markdown="1">

| enum         | Meaning                 |
|--------------|-------------------------|
| Required     | Input required          |
| MaxLength    | Maximum length          |
| MinLength    | Minimum length          |
| IntMax       | Maximum value (integer) |
| IntMin       | Minimum value (integer) |
| DoubleMax    | Maximum value (double)  |
| DoubleMin    | Minimum value (double)  |
| EmailAddress | Email address format    |
| Url          | URL format              |
| Date         | Date format             |
| Time         | Time format             |
| DateTime     | DateTime format         |
| Pattern      | Regular Expressions     |

</div><br>

Once you have set the rules, you can use them in the controller. Include the header file relating to this.<br>
The following code example validates the request data that is retrieved from the form. If you get a validation error, you get the error message.

```c++
QVariantMap blog = httpRequest().formItems("blog");
BlogValidator validator;
if (!validator.validate(blog)) {
    // Retrieve message rules for validation error
    QStringList errs = validator.errorMessages();
      :
}
```

Normally, since there are sets of multiple rules, there will also be multiple error messages. One-by-one processing is a little cumbersome. However, if you use the following method, you can export the all validation error messages at once (to be passed to the view):

```c++
exportValidationErrors(valid, "err_");
```

In the second argument, specify a prefix as the variable name for the export object.

##### In brief: Set rules for the form data and validate them by using validate().

## Custom validation

The explanation above is about static validation. It cannot be used in a dynamic case where the allowable range of the value changes according to what the value is. In this case, you may override the validate() method and then you can write any code of validation whatsoever.

The following code is an example on how a customized validation could look like:

```c++
bool FooValidator::validate(const QVariantMap &hash)
{
    bool ret = THashValidator::validate(hash);  // ←Validation of static rules
    if (ret) {
        QDate startDate = hash.value("startDate").toDate();
        QDate endDate = hash.value("endDate").toDate();

        if (endDate < startDate) {
            setValidationError("error");
            return false;
        }
          :
          :
    }
    return ret;
}
```

It compares the value of *endData* and the value of *startDate*. If the endDate is smaller than the startDate, an validation error will be thrown.