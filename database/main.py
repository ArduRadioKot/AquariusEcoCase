from flask import Flask, request
from . import db_sessyon

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret_key'


def main():
    db_sessyon.global_init("database/measurments.db")
    app.run()

if __name__ == '__main__':
    main()