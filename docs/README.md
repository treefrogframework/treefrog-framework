## TreeFrog Framework

This is the readMe. Here you can find useful links and information how to use Github Pages.

### Installs the jekyll.
```
 $ cd docs
 $ sudo apt install ruby ruby-dev ruby-all-dev make gcc bundler ruby-bundler
 $ sudo gem update
 $ bundle install
```

### Show Website in Browser

```
 localhost:4000
```

### Start Jekyll Build Command

```
 bundle exec jekyll build
```

### Start Jekyll Server Command

```
 bundle exec jekyll serve
```

### Start Jekyll Server and setting compiling configs

If several config-files are used (i.e. for production and development mode), use the following command in order to
let compile both config-files:

```
 bundle exec jekyll serve --config _config.yml,_config_dev.yml
 
```

### Start Jekyll Server Command with specific IP address

```
 bundle exec jekyll serve --host xxx.xxx.xxx.xxx
 
```

### Using Markdown inside an HTML \<div>-tag

```html
 <div markdown="1"> 
 
    ... markdown here ... 
    
 </div
```


### Useful Links

[Using Jekyll as a static site generator with GitHub Pages](https://help.github.com/articles/using-jekyll-as-a-static-site-generator-with-github-pages/){:target="_blank"}

[Jekyll Homepage](http://jekyllrb.com/){:target="_blank"}

[Tipue Search Engine](http://www.tipue.com/search/){:target="_blank"}

