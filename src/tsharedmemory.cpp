
/*!
  \class TSharedMemory
  \brief The TSharedMemory class provides access to a shared memory segment.
*/

/*!
  \fn TSharedMemory::TSharedMemory(const QString &name);
  Constructs a shared memory object with the given \a name.
*/

/*!
  \fn TSharedMemory::~TSharedMemory();
  Destructor.
*/

/*!
  \fn bool TSharedMemory::create(size_t size);
  Creates a shared memory segment of \a size bytes with the name passed to
  the constructor.
*/

/*!
  \fn void TSharedMemory::unlink();
  Attempts to unlink the shared memory with the name passed to
  the constructor.
*/

/*!
  \fn bool TSharedMemory::attach();
  Attach to the shared memory segment identified by the \a name that
  was passed to the constructor
*/

/*!
  \fn bool TSharedMemory::detach();
  Detaches from the shared memory segment.
*/

/*!
  \fn void *TSharedMemory::data();
  Returns a pointer to the contents of the shared memory segment,
  if one is attached
*/

/*!
  \fn const void * TSharedMemory::data() const;
  \sa data()
*/

/*!
  \fn QString TSharedMemory::name() const;
  Returns the name to this shared memory.
*/

/*!
  \fn TSharedMemory::size_t size() const;
  Returns the size of the attached shared memory segment.
*/

/*!
  \fn bool TSharedMemory::lockForRead();
  Locks the shared memory segment for reading by this process.
*/

/*!
  \fn bool TSharedMemory::lockForWrite();
  Locks the shared memory segment for writing by this process.
*/

/*!
  \fn bool TSharedMemory::unlock();
  Releases the lock on the shared memory segment.
*/
