# Инструкция по сборке iOS и загрузке в TestFlight

## Требования

1. **Apple Developer Account** (платный аккаунт, $99/год)
   - Зарегистрируйтесь на https://developer.apple.com
   - Убедитесь, что у вас есть активная подписка

2. **App Store Connect**
   - Создайте приложение в App Store Connect: https://appstoreconnect.apple.com
   - Bundle ID должен совпадать с `com.ecotest.app`

## Шаг 1: Настройка Apple Developer

1. Войдите в Apple Developer Portal
2. Создайте App ID с bundle identifier: `com.ecotest.app`
3. Создайте приложение в App Store Connect с тем же bundle ID

## Шаг 2: Сборка iOS приложения

Выполните команду:

```bash
npm run build:ios
```

При первом запуске вас попросят:
1. Войти в Apple Developer аккаунт (email и пароль)
2. Выбрать команду разработчиков (Team)
3. EAS автоматически создаст необходимые сертификаты и профили

**Важно:** Для сборки iOS приложения требуется:
- Активный Apple Developer аккаунт
- App ID должен быть создан в Apple Developer Portal
- Приложение должно быть создано в App Store Connect

## Шаг 3: Загрузка в TestFlight

После успешной сборки выполните:

```bash
npm run submit:ios
```

Или напрямую:

```bash
npx eas-cli submit --platform ios --profile production
```

EAS автоматически:
1. Найдет последнюю сборку
2. Загрузит её в App Store Connect
3. Обработает для TestFlight

## Альтернативный способ: Ручная загрузка

Если автоматическая загрузка не работает:

1. Скачайте `.ipa` файл со страницы сборки на expo.dev
2. Откройте App Store Connect
3. Перейдите в TestFlight
4. Загрузите `.ipa` файл через веб-интерфейс или Transporter

## Проверка статуса

После загрузки в TestFlight:
1. Зайдите в App Store Connect → TestFlight
2. Дождитесь обработки билда (обычно 10-30 минут)
3. Добавьте тестеров или используйте внутреннее тестирование
4. Тестеры получат приглашение на email

## Устранение проблем

### Ошибка: "No Apple account credentials found"
- Убедитесь, что вы вошли в Apple Developer аккаунт через EAS
- Выполните: `npx eas-cli credentials`

### Ошибка: "Bundle identifier not found"
- Создайте App ID в Apple Developer Portal
- Убедитесь, что bundle ID совпадает: `com.ecotest.app`

### Ошибка: "App not found in App Store Connect"
- Создайте приложение в App Store Connect
- Bundle ID должен совпадать с конфигурацией

## Полезные команды

```bash
# Просмотр учетных данных
npx eas-cli credentials

# Управление учетными данными iOS
npx eas-cli credentials --platform ios

# Просмотр статуса сборки
npx eas-cli build:list --platform ios

# Просмотр статуса загрузки
npx eas-cli submit:list --platform ios
```

