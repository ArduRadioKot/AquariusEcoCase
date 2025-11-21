import telebot

bot = telebot.TeleBot('токен')

@bot.message_handler(content_types=['text'])
def get_text_messages(message):
    if message.text == "/command1":
        temp = 22.7  # Температура (°C)
        humidity = 44  # Влажность (%)
        pressure_hpa = 1010  # Давление (ГПа)
        pressure_mmhg = round(pressure_hpa * 0.750062)  # Давление (мм рт.ст.)
        nh3 = 0.03  # Аммиак (ppm)
        no2 = 0.01  # NO2 (ppm)
        co2 = 410  # CO2 (ppm)

        msg = (
            f"Параметры воздуха:\n"
            f"Температура: {temp}°C\n"
            f"Влажность: {humidity}%\n"
            f"Атмосферное давление: {pressure_hpa} ГПа ({pressure_mmhg} мм рт.ст.)\n"
            f"Аммиак (NH3): {nh3} ppm\n"
            f"Оксид азота (NO2): {no2} ppm\n"
            f"Диоксид углерода (CO2): {co2} ppm"
        )
        bot.send_message(message.from_user.id, msg)
    elif message.text == "/help":
        bot.send_message(message.from_user.id, "Напиши /command1 для измерения параметров воздуха")
    else:
        bot.send_message(message.from_user.id, "Я тебя не понимаю. Напиши /help.")

bot.polling(none_stop=True, interval=0)