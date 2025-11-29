import sqlalchemy
from .db_sessyon import SqlAlchemyBase
from sqlalchemy import orm


class Measurement(SqlAlchemyBase):
    __tablename__ = 'measurements'

    id = sqlalchemy.Column(sqlalchemy.Integer,
                           primary_key=True, autoincrement=True)
    meanin = sqlalchemy.Column()
    indicator = sqlalchemy.Column(sqlalchemy.Integer,
                                  sqlalchemy.ForeignKey("indicators.id"))
    indic = orm.relationship('Indicator')
    device = sqlalchemy.Column(sqlalchemy.Integer,
                               sqlalchemy.ForeignKey("devices.id"))
    dev = orm.relationship('Device')