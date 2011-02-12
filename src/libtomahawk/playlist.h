#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>
#include <QList>
#include <QDebug>
#include <QVariant>
#include <QSharedPointer>

#include "query.h"
#include "typedefs.h"

#include "playlistinterface.h"

#include "dllmacro.h"

class DatabaseCommand_LoadAllPlaylists;
class DatabaseCommand_SetPlaylistRevision;
class DatabaseCommand_CreatePlaylist;

namespace Tomahawk
{

class DLLEXPORT PlaylistEntry : public QObject
{
Q_OBJECT
Q_PROPERTY( QString guid              READ guid         WRITE setGuid )
Q_PROPERTY( QString annotation        READ annotation   WRITE setAnnotation )
Q_PROPERTY( unsigned int duration     READ duration     WRITE setDuration )
Q_PROPERTY( unsigned int lastmodified READ lastmodified WRITE setLastmodified )
Q_PROPERTY( QVariant query            READ queryVariant WRITE setQueryVariant )

public:
    PlaylistEntry();
    virtual ~PlaylistEntry();
    
    void setQuery( const Tomahawk::query_ptr& q );
    const Tomahawk::query_ptr& query() const;
    
    // I wish Qt did this for me once i specified the Q_PROPERTIES:
    void setQueryVariant( const QVariant& v );
    QVariant queryVariant() const;

    QString guid() const { return m_guid; }
    void setGuid( const QString& s ) { m_guid = s; }

    QString annotation() const { return m_annotation; }
    void setAnnotation( const QString& s ) { m_annotation = s; }

    QString resultHint() const { return m_resulthint; }
    void setResultHint( const QString& s ) { m_resulthint= s; }

    unsigned int duration() const { return m_duration; }
    void setDuration( unsigned int i ) { m_duration = i; }

    unsigned int lastmodified() const { return m_lastmodified; }
    void setLastmodified( unsigned int i ) { m_lastmodified = i; }

    source_ptr lastSource() const;
    void setLastSource( source_ptr s );

private:    
    QString m_guid;
    Tomahawk::query_ptr m_query;
    QString m_annotation;
    unsigned int m_duration;
    unsigned int m_lastmodified;
    source_ptr   m_lastsource;
    QString      m_resulthint;
};


struct PlaylistRevision
{
    QString revisionguid;
    QString oldrevisionguid;
    QList<plentry_ptr> newlist;
    QList<plentry_ptr> added;
    QList<plentry_ptr> removed;
    bool applied; // false if conflict
};


class DLLEXPORT Playlist : public QObject, public PlaylistInterface
{
Q_OBJECT
Q_PROPERTY( QString guid            READ guid               WRITE setGuid )
Q_PROPERTY( QString currentrevision READ currentrevision    WRITE setCurrentrevision )
Q_PROPERTY( QString title           READ title              WRITE setTitle )
Q_PROPERTY( QString info            READ info               WRITE setInfo )
Q_PROPERTY( QString creator         READ creator            WRITE setCreator )
Q_PROPERTY( bool    shared          READ shared             WRITE setShared )

friend class ::DatabaseCommand_LoadAllPlaylists;
friend class ::DatabaseCommand_SetPlaylistRevision;
friend class ::DatabaseCommand_CreatePlaylist;

public:
    ~Playlist();
    
    static Tomahawk::playlist_ptr load( const QString& guid );

    // one CTOR is private, only called by DatabaseCommand_LoadAllPlaylists
    static Tomahawk::playlist_ptr create( const source_ptr& author,
                                         const QString& guid,
                                         const QString& title,
                                         const QString& info,
                                         const QString& creator,
                                         bool shared );

    static bool remove( const playlist_ptr& playlist );
    bool rename( const QString& title );

    virtual void loadRevision( const QString& rev = "" );

    const source_ptr& author();
    const QString& currentrevision()    { return m_currentrevision; }
    const QString& title()              { return m_title; }
    const QString& info()               { return m_info; }
    const QString& creator()            { return m_creator; }
    unsigned int lastmodified()         { return m_lastmodified; }
    const QString& guid()               { return m_guid; }
    bool shared() const                 { return m_shared; }

    const QList< plentry_ptr >& entries() { return m_entries; }
    virtual void addEntry( const Tomahawk::query_ptr& query, const QString& oldrev );
    virtual void addEntries( const QList<Tomahawk::query_ptr>& queries, const QString& oldrev );

    // <IGNORE hack="true">
    // these need to exist and be public for the json serialization stuff
    // you SHOULD NOT call them.  They are used for an alternate CTOR method from json.
    // maybe friend QObjectHelper and make them private?
    explicit Playlist( const source_ptr& author );
    void setCurrentrevision( const QString& s ) { m_currentrevision = s; }
    void setTitle( const QString& s )           { m_title = s; }
    void setInfo( const QString& s )            { m_info = s; }
    void setCreator( const QString& s )         { m_creator = s; }
    void setGuid( const QString& s )            { m_guid = s; }
    void setShared( bool b )                    { m_shared = b; }
    // </IGNORE>

    virtual QList<Tomahawk::query_ptr> tracks();

    virtual int unfilteredTrackCount() const { return m_entries.count(); }
    virtual int trackCount() const { return m_entries.count(); }

    virtual Tomahawk::result_ptr siblingItem( int itemsAway ) { return result_ptr(); }

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }
    
    virtual void setRepeatMode( PlaylistInterface::RepeatMode ) {}
    virtual void setShuffled( bool ) {}
    
    virtual void setFilter( const QString& pattern ) {}
    
signals:
    /// emitted when the playlist revision changes (whenever the playlist changes)
    void revisionLoaded( Tomahawk::PlaylistRevision );

    /// watch for this to see when newly created playlist is synced to DB (if you care)
    void created();

    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

public slots:
    // want to update the playlist from the model?
    // generate a newrev using uuid() and call this:
    void createNewRevision( const QString& newrev, const QString& oldrev, const QList< plentry_ptr >& entries );
    void reportCreated( const Tomahawk::playlist_ptr& self );
    void reportDeleted( const Tomahawk::playlist_ptr& self );

    void setRevision( const QString& rev,
                      const QList<QString>& neworderedguids,
                      const QList<QString>& oldorderedguids,
                      bool is_newest_rev,
                      const QMap< QString, Tomahawk::plentry_ptr >& addedmap,
                      bool applied );

    void resolve();

protected:
    // called from loadAllPlaylists DB cmd:
    explicit Playlist( const source_ptr& src,
                       const QString& currentrevision,
                       const QString& title,
                       const QString& info,
                       const QString& creator,
                       bool shared,
                       int lastmod,
                       const QString& guid = "" ); // populate db

    // called when creating new playlist
    explicit Playlist( const source_ptr& author,
                       const QString& guid,
                       const QString& title,
                       const QString& info,
                       const QString& creator,
                       bool shared );
    
    QList< plentry_ptr > newEntries( const QList< plentry_ptr >& entries );
    PlaylistRevision setNewRevision( const QString& rev,
                                     const QList<QString>& neworderedguids,
                                     const QList<QString>& oldorderedguids,
                                     bool is_newest_rev,
                                     const QMap< QString, Tomahawk::plentry_ptr >& addedmap );
    
    QList<plentry_ptr> addEntriesInternal( const QList<Tomahawk::query_ptr>& queries );
    
private slots:
    void onResultsFound( const QList<Tomahawk::result_ptr>& results );
    void onResolvingFinished();
    
private:
    Playlist();
    void init();
    
    source_ptr m_source;
    QString m_currentrevision;
    QString m_guid, m_title, m_info, m_creator;
    unsigned int m_lastmodified;
    bool m_shared;

    QList< plentry_ptr > m_entries;
    bool m_locallyChanged;

};

};

#endif // PLAYLIST_H
