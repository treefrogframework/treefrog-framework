import os
import re
from pathlib import Path
from typing import Any, Dict, Iterable, List, Sequence, Tuple, Union

from docutils import nodes
from docutils.nodes import Node, document
from sphinx.application import Sphinx

from docutils.parsers.rst import Directive

# Do not require newer sphinx. EPEL packages build man pages with Sphinx 1.7.6. Refer: CDRIVER-4767
needs_sphinx = '1.7'
author = 'MongoDB, Inc'

# -- Options for HTML output ----------------------------------------------

smart_quotes = False
html_show_sourcelink = False

# Note: http://www.sphinx-doc.org/en/1.5.1/config.html#confval-html_copy_source
# This will degrade the Javascript quicksearch if we ever use it.
html_copy_source = False


def _file_man_page_name(fpath: Path) -> Union[str, None]:
    "Given an rST file input, find the :man_page: frontmatter value, if present"
    lines = fpath.read_text().splitlines()
    for line in lines:
        mat = re.match(r':man_page:\s+(.+)', line)
        if not mat:
            continue
        return mat[1]


def _collect_man(app: Sphinx):
    # Note: 'app' is partially-formed, as this is called from the Sphinx.__init__
    docdir = Path(app.srcdir)
    # Find everything:
    children = docdir.rglob('*')
    # Find only regular files:
    files = filter(Path.is_file, children)
    # Find files that have a .rst extension:
    rst_files = (f for f in files if f.suffix == '.rst')
    # Pair each file with its :man_page: frontmatter, if present:
    with_man_name = ((f, _file_man_page_name(f)) for f in rst_files)
    # Filter out pages that do not have a :man_page: item:s
    pairs: Iterable[tuple[Path, str]] = ((f, m) for f, m in with_man_name if m)
    # Populate the man_pages:
    for filepath, man_name in pairs:
        # Generate the docname.
        relative_path = filepath.relative_to(app.srcdir)
        # The docname is relative to the source directory, and excludes the suffix.
        docname = str(relative_path.parent / filepath.stem)

        assert docname, filepath
        man_pages.append((docname, man_name, '', [author], 3))


# -- Options for manual page output ---------------------------------------

# NOTE: This starts empty, but we populate it in `setup` in _collect_man() (see above)
man_pages: List[Tuple[str, str, str, List[str], int]] = []

# If true, show URL addresses after external links.
#
# man_show_urls = False

# -- Sphinx customization ---------------------------------------


def add_ga_javascript(app: Sphinx, pagename: str, templatename: str, context: Dict[str, Any], doctree: document):
    if not app.env.config.analytics:
        return

    # Add google analytics.
    context['metatags'] = (
        context.get('metatags', '')
        + """
<!-- Google tag (gtag.js) -->
<script async src="https://www.googletagmanager.com/gtag/js?id=G-56KD6L3MDX"></script>
<script>
  window.dataLayer = window.dataLayer || [];
  function gtag(){dataLayer.push(arguments);}
  gtag('js', new Date());

  gtag('config', 'G-56KD6L3MDX');
</script>
"""
    )


class VersionList(Directive):
    """Custom directive to generate the version list from an environment variable"""

    option_spec = {}
    has_content = True

    def run(self) -> Sequence[Node]:
        if self.content[0] != 'libmongoc' and self.content[0] != 'libbson':
            print('versionlist must be libmongoc or libbson')
            return []

        libname = self.content[0]
        env_name = libname.upper() + '_VERSION_LIST'
        if env_name not in os.environ:
            print(env_name + ' not set, not generating version list')
            return []

        versions = os.environ[env_name].split(',')

        header = nodes.paragraph('', '')
        p = nodes.paragraph('', '')
        uri = 'https://www.mongoc.org/%s/%s/index.html' % (libname, versions[0])
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
            uri = 'https://www.mongoc.org/%s/%s/index.html' % (libname, v)
            p += nodes.reference('', v, internal=False, refuri=uri)
            item += p
            blist += item
        return [header, blist]


def mongoc_common_setup(app: Sphinx):
    _collect_man(app)
    app.connect('html-page-context', add_ga_javascript)
    # Run sphinx-build -D analytics=1 to enable Google Analytics.
    app.add_config_value('analytics', False, 'html')
    app.add_directive('versionlist', VersionList)
