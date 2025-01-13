#!/usr/bin/env python3

import cgi
import cgitb

cgitb.enable()

print("Content-Type: text/html")
print()
print("<html><head><title>CGI Example</title></head><body>")
print("<h1>Hello from CGI!</h1>")

form = cgi.FieldStorage()
name = form.getvalue("name")

if name:
    print(f"<p>Hello, {name}!</p>")
else:
    print("<p>Please enter your name below:</p>")
    print('<form method="post" action="/cgi-bin/example.cgi">')
    print('<input type="text" name="name">')
    print('<input type="submit" value="Submit">')
    print('</form>')

print("</body></html>")