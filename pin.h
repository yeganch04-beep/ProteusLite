#ifndef PIN_H
#define PIN_H

#include <QPoint>
#include <QString>

enum class PinType
{
    Input,
    Output,
    Bidirectional
};

enum class LogicState
{
    Undefined,
    Low,
    High
};

class Pin
{
public:
    Pin(const QString &name, PinType type, const QPoint &offset);

    QString name() const;
    PinType type() const;
    QPoint offset() const;
    LogicState state() const;
    QString connectedNodeId() const;

    void setState(LogicState state);
    void setConnectedNodeId(const QString &nodeId);

private:
    QString m_name;
    PinType m_type;
    QPoint m_offset;
    LogicState m_state;
    QString m_connectedNodeId;
};

#endif // PIN_H
