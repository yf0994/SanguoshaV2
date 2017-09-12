#ifdef AUDIO_SUPPORT

#include <QObject>
#include <QCache>
#include <QTime>
#include <QStringList>
#include <QSet>

#include "audio.h"
#include "fmod.h"
#include "util.h"

static FMOD_SYSTEM *System = NULL;
static FMOD_SOUNDGROUP *EffectGroup = NULL;
static FMOD_SOUNDGROUP *BackgroundMusicGroup = NULL;

class Sound
{
public:
    explicit Sound(const QString &fileName, bool backgroundMusic = false)
        : m_sound(NULL), m_channel(NULL)
    {
        FMOD_MODE mode = FMOD_DEFAULT;
        FMOD_SOUNDGROUP *soundGroup = EffectGroup;
        if (backgroundMusic) {
            mode |= FMOD_CREATESTREAM;
            soundGroup = BackgroundMusicGroup;
        }

        FMOD_System_CreateSound(System, fileName.toAscii(),
            mode, NULL, &m_sound);
        FMOD_Sound_SetSoundGroup(m_sound, soundGroup);
        FMOD_System_Update(System);
    }

    ~Sound()
    {
        FMOD_Sound_Release(m_sound);
        FMOD_System_Update(System);
    }

    void play(bool loop = false)
    {
        if (loop) {
            FMOD_Sound_SetMode(m_sound, FMOD_LOOP_NORMAL);
        }

        FMOD_System_PlaySound(System, FMOD_CHANNEL_FREE,
            m_sound, false, &m_channel);
        FMOD_System_Update(System);
    }

    void stop()
    {
        if (NULL != m_channel) {
            FMOD_Channel_Stop(m_channel);
            FMOD_System_Update(System);

            m_channel = NULL;
        }
    }

    bool isPlaying() const
    {
        FMOD_BOOL playing = false;
        if (NULL != m_channel) {
            FMOD_Channel_IsPlaying(m_channel, &playing);
        }
        return playing;
    }

private:
    Q_DISABLE_COPY(Sound)

    FMOD_SOUND *m_sound;
    FMOD_CHANNEL *m_channel;
};

class BackgroundMusicPlayList
{
public:
    enum PlayOrder {
        Sequential = 1,
        Shuffle = 2,
    };

    explicit BackgroundMusicPlayList(const QStringList &fileNames,
        BackgroundMusicPlayList::PlayOrder order = Sequential)
        : m_fileNames(fileNames), m_order(order), m_index(-1)
    {
    }

    int count() const { return m_fileNames.size(); }

    bool operator==(const BackgroundMusicPlayList &other) const
    {
        if (this == &other) {
            return true;
        }

        if (this->m_order != other.m_order) {
            return false;
        }

        switch (other.m_order) {
        case Sequential:
            return this->m_fileNames == other.m_fileNames;

        case Shuffle:
            return this->m_fileNames.toSet() == other.m_fileNames.toSet();

        default:
            return false;
        }
    }
    bool operator!=(const BackgroundMusicPlayList &other) const { return !(*this == other); }

    QString nextFileName()
    {
        if (Shuffle == m_order) {
            if (m_randomQueue.isEmpty()) {
                fillRandomQueue();
            }

            QString fileName = m_randomQueue.takeFirst();
            m_index = m_fileNames.indexOf(fileName);
            return fileName;
        }
        else {
            if (++m_index >= m_fileNames.size()) {
                m_index = 0;
            }
            return m_fileNames.at(m_index);
        }
    }

private:
    void fillRandomQueue()
    {
        m_randomQueue = m_fileNames;

        qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
        qShuffle(m_randomQueue);
    }

private:
    QStringList m_fileNames;
    QStringList m_randomQueue;
    PlayOrder m_order;
    int m_index;
};

class BackgroundMusicPlayer : public QObject
{
public:
    BackgroundMusicPlayer() : m_timer(0), m_count(0) {}

    void play(const QString &fileNames, bool random)
    {
        if (m_timer != 0) {
            return;
        }

        {
            BackgroundMusicPlayList::PlayOrder playOrder
                = random ? BackgroundMusicPlayList::Shuffle : BackgroundMusicPlayList::Sequential;
            QScopedPointer<BackgroundMusicPlayList> playList(new BackgroundMusicPlayList(fileNames.split(";"), playOrder));
            if (!m_playList || (*m_playList != *playList)) {
                m_playList.swap(playList);
                m_count = m_playList->count();
            }
        }

        playNext();

        if (m_count > 1) {
            m_timer = startTimer(m_interval);
        }
    }

    void stop()
    {
        if (m_timer != 0) {
            killTimer(m_timer);
            m_timer = 0;
        }
    }

    void shutdown() { m_sound.reset(); }

protected:
    virtual void timerEvent(QTimerEvent *)
    {
        if (!m_sound->isPlaying()) {
            playNext();
        }
    }

private:
    void playNext()
    {
        m_sound.reset(new Sound(m_playList->nextFileName(), true));
        m_sound->play(1 == m_count);
    }

    Q_DISABLE_COPY(BackgroundMusicPlayer)

private:
    QScopedPointer<BackgroundMusicPlayList> m_playList;
    QScopedPointer<Sound> m_sound;
    int m_timer;
    int m_count;

    static const int m_interval = 500;
};

static QCache<QString, Sound> SoundCache;
static BackgroundMusicPlayer backgroundMusicPlayer;
QString Audio::m_customBackgroundMusicFileName;

void Audio::init()
{
    if (NULL == System) {
        if (FMOD_OK == FMOD_System_Create(&System)) {
            FMOD_System_Init(System, MAX_CHANNEL_COUNT, FMOD_INIT_NORMAL, NULL);

            FMOD_System_CreateSoundGroup(System, "Effects", &EffectGroup);
            FMOD_System_CreateSoundGroup(System, "BackgroundMusics", &BackgroundMusicGroup);
            FMOD_SoundGroup_SetMaxAudible(BackgroundMusicGroup, 1);
            FMOD_SoundGroup_SetMaxAudibleBehavior(BackgroundMusicGroup, FMOD_SOUNDGROUP_BEHAVIOR_MUTE);
            FMOD_SoundGroup_SetMuteFadeSpeed(BackgroundMusicGroup, 2);
        }
    }
}

void Audio::quit()
{
    if (NULL != System) {
        stopAll();

        FMOD_SoundGroup_Release(EffectGroup);
        FMOD_SoundGroup_Release(BackgroundMusicGroup);
        EffectGroup = NULL;
        BackgroundMusicGroup = NULL;

        //QCache::clear���Զ�delete���е�Sound����
        SoundCache.clear();
        backgroundMusicPlayer.shutdown();

        FMOD_System_Release(System);
        System = NULL;
    }
}

void Audio::play(const QString &fileName, bool continuePlayWhenPlaying/* = false*/)
{
    if (NULL != System) {
        Sound *sound = SoundCache[fileName];
        if (NULL == sound) {
            sound = new Sound(fileName);
            SoundCache.insert(fileName, sound);
        }
        else if (!continuePlayWhenPlaying && sound->isPlaying()) {
            return;
        }

        sound->play();
    }
}

void Audio::setEffectVolume(float volume)
{
    FMOD_SoundGroup_SetVolume(EffectGroup, volume);
}

void Audio::setBackgroundMusicVolume(float volume)
{
    FMOD_SoundGroup_SetVolume(BackgroundMusicGroup, volume);
}

void Audio::playBackgroundMusic(const QString &fileNames, bool random/* = false*/)
{
    if (NULL != System) {
        if (!m_customBackgroundMusicFileName.isEmpty()) {
            backgroundMusicPlayer.play(m_customBackgroundMusicFileName, random);
        }
        else {
            backgroundMusicPlayer.play(fileNames, random);
        }
    }
}

void Audio::stopBackgroundMusic()
{
    backgroundMusicPlayer.stop();

    while (isBackgroundMusicPlaying()) {
        FMOD_SoundGroup_Stop(BackgroundMusicGroup);
    }
}

bool Audio::isBackgroundMusicPlaying()
{
    int numPlaying = 0;
    FMOD_SoundGroup_GetNumPlaying(BackgroundMusicGroup, &numPlaying);
    return numPlaying > 0;
}

void Audio::stopAll()
{
    FMOD_SoundGroup_Stop(EffectGroup);
    stopBackgroundMusic();

    resetCustomBackgroundMusicFileName();
}

QString Audio::getVersion()
{
    /*
     * FMOD version number.
     * 0xaaaabbcc -> aaaa = major version number.
     * bb = minor version number.
     * cc = development version number.
    */
    unsigned int version = 0;
    if (NULL != System && FMOD_OK == FMOD_System_GetVersion(System, &version)) {
        return QString("%1.%2.%3").arg((version & 0xFFFF0000) >> 16, 0, 16)
            .arg((version & 0xFF00) >> 8, 2, 16, QChar('0'))
            .arg((version & 0xFF), 2, 16, QChar('0'));
    }

    return "";
}

#endif // AUDIO_SUPPORT
