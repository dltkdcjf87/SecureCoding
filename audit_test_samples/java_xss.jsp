<%@ page language="java" contentType="text/html; charset=UTF-8" pageEncoding="UTF-8" %>
    <!DOCTYPE html>
    <html>

    <head>
        <title>XSS Test</title>
    </head>

    <body>
        <h1>Welcome, <%= request.getParameter("name") %>
        </h1>
        <!-- XSS Vulnerability: directly rendering request parameter without escaping -->
    </body>

    </html>