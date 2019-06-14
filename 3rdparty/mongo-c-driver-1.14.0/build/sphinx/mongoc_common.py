import os

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

    context['metatags'] = context.get('metatags', '') + """<script>
  (function(w,d,s,l,i){w[l]=w[l]||[];w[l].push(
      {'gtm.start': new Date().getTime(),event:'gtm.js'}
    );var f=d.getElementsByTagName(s)[0],
    j=d.createElement(s),dl=l!='dataLayer'?'&l='+l:'';j.async=true;j.src=
    '//www.googletagmanager.com/gtm.js?id='+i+dl;f.parentNode.insertBefore(j,f);
    })(window,document,'script','dataLayer','GTM-JQHP');
</script>"""


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

    return matches[0]


def mongoc_common_setup(app):
    app.connect('doctree-read', process_nodes)
    app.connect('env-updated', create_nojekyll)
    app.connect('html-page-context', add_ga_javascript)
    # Run sphinx-build -D analytics=1 to enable Google Analytics.
    app.add_config_value('analytics', False, 'html')
