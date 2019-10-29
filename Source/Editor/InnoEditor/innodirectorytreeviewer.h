#ifndef INNODIRECTORYTREEVIEWER_H
#define INNODIRECTORYTREEVIEWER_H

#include <QTreeView>
#include <QFileSystemModel>

class InnoDirectoryTreeViewer : public QTreeView
{
	Q_OBJECT
public:
    explicit InnoDirectoryTreeViewer(QWidget *parent = nullptr);

    void Initialize();

    QString GetRootPath();
    void SetRootPath(QString path);
    void SetActivePath(QString path);
    QString GetFilePath(QModelIndex index);

private:
	QFileSystemModel* m_dirModel;

signals:

public slots:
};

#endif // INNODIRECTORYTREEVIEWER_H
