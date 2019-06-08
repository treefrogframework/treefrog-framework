"""
Adapted from the sphinxcontrib.taglist extension, Copyright 2007-2009 by the
Sphinx team.

                              MIT LICENSE

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
"""

from docutils import nodes, utils
from sphinx.environment import NoUri
try:
    from sphinx.util.compat import Directive
except ImportError:
    from docutils.parsers.rst import Directive


def get_tags(s):
    return list(filter(lambda x: x,
                       map(lambda x: x.strip(),
                           s.split(' ')
                           )
                       )
                )


def tag_role(name, rawtext, text, lineno, inliner, options=None, content=[]):
    status = utils.unescape(text.strip().replace(' ', '_'))
    return [], []


def find_node(doctree, klass):
    matches = doctree.traverse(lambda node: isinstance(node, klass))
    if not matches:
        raise IndexError("No %s in %s" % (klass, doctree))

    return matches[0]


class taglist(nodes.General, nodes.Element):
    def __init__(self, *a, **b):
        super(taglist, self).__init__(*a, **b)
        self.tags = []


class TaglistDirective(Directive):
    has_content = True

    option_spec = {
        'tags': str,
    }

    def run(self):
        tl = taglist('')
        tl.tags = get_tags(self.options.get('tags', ''))

        section = nodes.section(ids=['taglist'])
        title = nodes.Text(self.content[0] or "See Also:")
        section += nodes.title(title, title)

        text = nodes.paragraph()
        text += [nodes.Text(":tag:`%s`"%t) for t in tl.tags ]
        self.state.nested_parse(text, 0, section)

        section += [tl]
        return [section]


def process_tags(app, doctree):
    # collect all tags in the environment
    # this is not done in the directive itself because it some transformations
    # must have already been run, e.g. substitutions
    env = app.builder.env
    if not hasattr(env, 'tags_all_tags'):
        env.tags_all_tags = []

    metadata = env.metadata[env.docname]

    # A page like mongoc_session_opts_get_causal_consistency.rst sets its
    # tags with ":tags: session"
    tags = metadata.get('tags')
    if tags:
        env.tags_all_tags.append({'docname': env.docname,
                                  'tags': get_tags(tags)})

    for node in doctree.traverse(taglist):
        env.tags_all_tags.append({'docname': env.docname, 'tags': node.tags})


def process_taglist_nodes(app, doctree, fromdocname):
    # Replace all taglist nodes with a list of the collected tags.
    # Augment each tag with a backlink to the original location.
    env = app.builder.env

    if not hasattr(env, 'tags_all_tags'):
        env.tags_all_tags = []

    for node in doctree.traverse(taglist):
        links = set()

        for tag_info in env.tags_all_tags:
            tags = tag_info['tags']
            if not set(tags).intersection(node.tags):
                continue

            if fromdocname == tag_info['docname']:
                continue

            links.add(tag_info['docname'])

        content = []
        for docname in sorted(links):
            # (Recursively) resolve references in the tag content
            para = nodes.paragraph(classes=['tag-source'])
            refnode = nodes.reference('', '', internal=True)
            try:
                refnode['refuri'] = app.builder.get_relative_uri(
                    fromdocname, docname)
            except NoUri:
                # ignore if no URI can be determined, e.g. for LaTeX output
                pass

            # Create a reference
            refnode.append(nodes.Text(docname))
            para += refnode
            content.append(para)

        node.replace_self(content)


def purge_tags(app, env, docname):
    if not hasattr(env, 'tags_all_tags'):
        return
    env.tags_all_tags = [tag for tag in env.tags_all_tags
                         if tag['docname'] != docname]


def visit_tag_node(self, node):
    pass


def depart_tag_node(self, node):
    pass


def setup(app):
    app.add_role('tag', tag_role)
    app.add_node(taglist)
    app.add_directive('taglist', TaglistDirective)
    app.connect('doctree-read', process_tags)
    app.connect('doctree-resolved', process_taglist_nodes)
    app.connect('env-purge-doc', purge_tags)

    return {'parallel_write_safe': True, 'parallel_read_safe': False}
