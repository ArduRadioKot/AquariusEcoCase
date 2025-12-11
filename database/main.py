import sqlite3
import csv


class DB:
    def __init__(self):
        self.connection = sqlite3.connect('measurments.db')
        self.cursor = self.connection.cursor()

    def get(self, dat: str):
        ind_id = self.cursor.execute("""SELECT id FROM indicators WHERE indicator = ?""", (dat,)).fetchone()

        if ind_id is None:
            return None

        results = self.cursor.execute("""SELECT id, meaning FROM measurements WHERE indicator = ?""",
                                      (ind_id[0],)).fetchall()

        with open("ans.csv", 'w', encoding='utf-8', newline='') as f:
            cs = csv.writer(f)
            cs.writerow(['id', 'meaning'])
            cs.writerows(results)

        return 'ans.csv'

    def nem_m(self, id_ind: int, id_dev: int, mea: str):
        self.cursor.execute("""INSERT INTO measurements (meaning, indicator, device) VALUES (?, ?, ?)""",
                            (mea, id_ind, id_dev))
        self.connection.commit()

    def close(self):
        self.connection.close()
