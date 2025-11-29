import sqlalchemy
from .db_sessyon import SqlAlchemyBase


class Device(SqlAlchemyBase):
    __tablename__ = 'devices'

    id = sqlalchemy.Column(sqlalchemy.Integer,
                           primary_key=True, autoincrement=True)
    location = sqlalchemy.Column(nullable=False)
