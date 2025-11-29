import sqlalchemy
from .db_sessyon import SqlAlchemyBase


class Indicator(SqlAlchemyBase):
    __tablename__ = 'indicators'

    id = sqlalchemy.Column(sqlalchemy.Integer,
                           primary_key=True, autoincrement=True)
    indicator = sqlalchemy.Column(nullable=False)
