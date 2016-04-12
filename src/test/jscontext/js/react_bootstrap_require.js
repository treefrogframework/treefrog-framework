var ReactBootstrap = require('react-bootstrap');
var React = require('react');
var ReactDOMServer = require('react-dom-server');

var Button = ReactBootstrap.Button;
ReactDOMServer.renderToString(React.createElement(Button))
