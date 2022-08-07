import os
from docutils import nodes
try:
    from sphinx.util.compat import Directive
except ImportError:
    from docutils.parsers.rst import Directive

needs_sphinx = '1.6'
author = 'MongoDB, Inc'

# -- Options for HTML output ----------------------------------------------

smart_quotes = False
html_show_sourcelink = False

# Note: http://www.sphinx-doc.org/en/1.5.1/config.html#confval-html_copy_source
# This will degrade the Javascript quicksearch if we ever use it.
html_copy_source = False

# -- Options for manual page output ---------------------------------------

# HACK: Just trick Sphinx's ManualPageBuilder into thinking there are pages
# configured - we'll do it dynamically in process_nodes.
man_pages = [True]

# If true, show URL addresses after external links.
#
# man_show_urls = False

# -- Sphinx customization ---------------------------------------

from docutils.nodes import title


# To publish HTML docs at GitHub Pages, create .nojekyll file. In Sphinx 1.4 we
# could use the githubpages extension, but old Ubuntu still has Sphinx 1.3.
def create_nojekyll(app, env):
    if app.builder.format == 'html':
        path = os.path.join(app.builder.outdir, '.nojekyll')
        with open(path, 'wt') as f:
            f.write('foo')


def add_ga_javascript(app, pagename, templatename, context, doctree):
    if not app.env.config.analytics:
        return

    # Add google analytics and NPS survey.
    context['metatags'] = context.get('metatags', '') + """
<!-- Global site tag (gtag.js) - Google Analytics -->
<script async src="https://www.googletagmanager.com/gtag/js?id=UA-7301842-14"></script>
<script>
  window.dataLayer = window.dataLayer || [];
  function gtag(){dataLayer.push(arguments);}
  gtag('js', new Date());

  gtag('config', 'UA-7301842-14');
</script>
<!--  NPS survey -->
<script type="text/javascript">
  !function(e,t,r,n,a){if(!e[a]){for(var i=e[a]=[],s=0;s<r.length;s++){var c=r[s];i[c]=i[c]||function(e){return function(){var t=Array.prototype.slice.call(arguments);i.push([e,t])}}(c)}i.SNIPPET_VERSION="1.0.1";var o=t.createElement("script");o.type="text/javascript",o.async=!0,o.src="https://d2yyd1h5u9mauk.cloudfront.net/integrations/web/v1/library/"+n+"/"+a+".js";var l=t.getElementsByTagName("script")[0];l.parentNode.insertBefore(o,l)}}(window,document,["survey","reset","config","init","set","get","event","identify","track","page","screen","group","alias"],"Dk30CC86ba0nATlK","delighted");

  delighted.survey();
</script>
"""


def process_nodes(app, doctree):
    if man_pages == [True]:
        man_pages.pop()

    env = app.env
    metadata = env.metadata[env.docname]

    # A page like installing.rst sets its name with ":man_page: bson_installing"
    page_name = metadata.get('man_page')
    if not page_name:
        print('Not creating man page for %s' % env.docname)
        return

    page_title = find_node(doctree, title)

    man_pages.append((env.docname, page_name, page_title.astext(), [author], 3))


def find_node(doctree, klass):
    matches = doctree.traverse(lambda node: isinstance(node, klass))
    if not matches:
        raise IndexError("No %s in %s" % (klass, doctree))

    return list(matches)[0]

class VersionList(Directive):
    """ Custom directive to generate the version list from an environment variable """
    option_spec = {}
    has_content = True
    def run(self):        
        if self.content[0] != 'libmongoc' and self.content[0] != 'libbson':
            print ('versionlist must be libmongoc or libbson')
            return []

        libname = self.content[0]
        env_name = libname.upper() + '_VERSION_LIST'
        if env_name not in os.environ:
            print (env_name + ' not set, not generating version list')
            return []
        
        versions = os.environ[env_name].split(',')

        header = nodes.paragraph('','')
        p = nodes.paragraph('', '')
        uri = 'http://mongoc.org/%s/%s/index.html' % (libname, versions[0])
        p += nodes.reference('', 'Latest Release (%s)' % versions[0], internal=False, refuri=uri)
        header += p
        p = nodes.paragraph('', '')
        uri = 'https://s3.amazonaws.com/mciuploads/mongo-c-driver/docs/%s/latest/index.html' % (libname)
        p += nodes.reference('', 'Current Development (master)', internal=False, refuri=uri)
        header += p

        blist = nodes.bullet_list()
        for v in versions:
            item = nodes.list_item()
            p = nodes.paragraph('', '')
            uri = 'http://mongoc.org/%s/%s/index.html' % (libname, v)
            p += nodes.reference('', v, internal=False, refuri=uri)
            item += p
            blist += item
        return [header, blist]


def mongoc_common_setup(app):
    app.connect('doctree-read', process_nodes)
    app.connect('env-updated', create_nojekyll)
    app.connect('html-page-context', add_ga_javascript)
    # Run sphinx-build -D analytics=1 to enable Google Analytics.
    app.add_config_value('analytics', False, 'html')
    app.add_directive ('versionlist', VersionList)