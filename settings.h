#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Cache : public QSettings
{
public:
    static const QString GREETER_DATA_DIR_PATH;
    static void prepare();

    Cache() : QSettings(GREETER_DATA_DIR_PATH + "/state", QSettings::NativeFormat) {}
    QString getLastUser() { return value("last-user").toString(); }
    void setLastUser(QString userId) { setValue("last-user", userId); }
    QString getLastSession(QString userId) { return value(userId + "/last-session").toString(); }
    void setLastSession(QString userId, QString session) { setValue(userId + "/last-session", session); }
};

#define CONFIG_FILE "/etc/lightdm/lightdm-qt5-greeter.conf"
#define GREETER_ICON_THEME "greeter-icon-theme"
#define BACKGROUND_IMAGE_KEY "greeter-background-image"
#define LOGINFORM_OFFSETX_KEY "loginform-offset-x"
#define LOGINFORM_OFFSETY_KEY "loginform-offset-y"
#define LOGINFORM_SHOW_INPUT_CHAIN "loginform-show-input-chain"
#define CCS_TEAM_NAME "ccs-team-name"
#define CCS_TEAM_LABEL_COLOR "ccs-team-label-color"
#define CCS_TEAM_LABEL_SHADOW_COLOR "ccs-team-label-shadow-color"
#define CCS_CONTEST_API_URL "ccs-contest-api-url"
#define CCS_AUTOLOGIN_USERNAME "ccs-autologin-username"
#define CCS_AUTOLOGIN_PASSWORD "ccs-autologin-password"
#define CCS_START_MINIMUM_MSEC "ccs-start-minimum-msec"

class Settings : public QSettings
{
public:
    Settings() : QSettings(QString(CONFIG_FILE), QSettings::NativeFormat) {}
    QString iconThemeName() { return value(GREETER_ICON_THEME).toString(); }
    QString backgrundImagePath() { return value(BACKGROUND_IMAGE_KEY).toString(); }
    QString offsetX() { return value(LOGINFORM_OFFSETX_KEY).toString(); }
    QString offsetY() { return value(LOGINFORM_OFFSETY_KEY).toString(); }
    QStringList loginformShowInputChain() { return value(LOGINFORM_SHOW_INPUT_CHAIN).toStringList(); }
    QString ccsTeamName() { return value(CCS_TEAM_NAME).toString(); }
    QString ccsTeamLabelColor() { return value(CCS_TEAM_LABEL_COLOR).toString(); }
    QString ccsTeamLabelShadowColor() { return value(CCS_TEAM_LABEL_SHADOW_COLOR).toString(); }
    QString ccsContestApiUrl() { return value(CCS_CONTEST_API_URL).toString(); }
    QString ccsAutologinUsername() { return value(CCS_AUTOLOGIN_USERNAME).toString(); }
    QString ccsAutologinPassword() { return value(CCS_AUTOLOGIN_PASSWORD).toString(); }
    int ccsStartMinimumMsec() { return value(CCS_START_MINIMUM_MSEC, 500).toInt(); }
};

#endif // SETTINGS_H
