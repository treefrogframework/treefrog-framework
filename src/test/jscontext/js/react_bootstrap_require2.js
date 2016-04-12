var React = require('react');
var ReactDOMServer = require('react-dom-server');

// tutorial1-raw.js
var CommentBox = React.createClass({
  displayName: 'CommentBox',
  render: function() {
    return (
      React.createElement('div', {className: "commentBox"},
        "Hello, world! I am a CommentBox."
      )
    );
  }
});

ReactDOMServer.renderToString(React.createElement(CommentBox, null));
