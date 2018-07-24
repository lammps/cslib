! parallel server example in Fortran

! main program

program server_parallel

USE ISO_C_binding
USE cslib_wrap

implicit none
include 'mpif.h'

INTEGER :: i,j,m,ierr
INTEGER :: onlyserver,me,nprocs,mpifree
INTEGER :: nlen,nlocal,nper
integer :: argc
TYPE(C_ptr) :: cs,ptr,pfieldID,pfieldtype,pfieldlen
CHARACTER(len=32) :: mode,arg
INTEGER :: msgID,nfield

INTEGER, TARGET :: world,both

INTEGER(C_INT), POINTER :: fieldID(:) => NULL()
INTEGER(C_INT), POINTER :: fieldtype(:) => NULL()
INTEGER(C_INT), POINTER :: fieldlen(:) => NULL()

INTEGER(4), ALLOCATABLE, target :: ids(:)

INTEGER(4), ALLOCATABLE, target :: sint(:)
INTEGER(8), ALLOCATABLE, target :: sint64(:)
REAL(4), ALLOCATABLE, target :: sfloat(:)
REAL(8), ALLOCATABLE, target :: sdouble(:)
REAL(8), ALLOCATABLE, target :: sarray(:,:)

INTEGER(4), ALLOCATABLE, target :: rint(:)
INTEGER(8), ALLOCATABLE, target :: rint64(:)
REAL(4), ALLOCATABLE, target :: rfloat(:)
REAL(8), ALLOCATABLE, target :: rdouble(:)
REAL(8), ALLOCATABLE, target :: rarray(:,:)

integer(4) :: oneint
integer(8) :: oneint64
real(4) :: onefloat
real(8) :: onedouble

CHARACTER(len=32) :: sstring
CHARACTER(c_char), POINTER :: rstring(:) => NULL()

! MPI setup

call MPI_INIT(ierr)
world = MPI_COMM_WORLD
call MPI_COMM_RANK(world,me,ierr)
call MPI_COMM_SIZE(world,nprocs,ierr)

! command-line args

argc = command_argument_count()

IF (argc /= 2) THEN
  IF (me == 0) THEN
    PRINT *,"Syntax: mpirun -np P server_parallel_f90 mode N"
    PRINT *,"  mode = file or zmq or mpi/one or mpi/two"
    PRINT *,"  N = length of data vectors"
  ENDIF
  call exit(1)
endif

CALL GET_COMMAND_ARGUMENT(1,mode)
CALL GET_COMMAND_ARGUMENT(2,arg)
READ (arg,"(i10)") nlen

! setup messaging

mpifree = 0

IF (mode == "file") THEN
  CALL cslib_open_fortran(1,TRIM(mode)//C_null_char, &
          "tmp.couple"//C_null_char,c_loc(world),cs)
else if (mode == "zmq") then
  CALL cslib_open_fortran(1,TRIM(mode)//C_null_char, &
          "*:5555"//C_null_char,c_loc(world),cs)
ELSE IF (mode == "mpi/one") THEN
  mpifree = 1
  CALL MPI_Comm_split(world,1,me,onlyserver,ierr)
  CALL MPI_Comm_rank(onlyserver,me,ierr)
  CALL MPI_Comm_size(onlyserver,nprocs,ierr)
  both = world
  world = onlyserver
  CALL cslib_open_fortran_mpi_one(1,TRIM(mode)//C_null_char, &
          c_loc(both),c_loc(world),cs)
ELSE IF (mode == "mpi/two") THEN
  CALL cslib_open_fortran(1,TRIM(mode)//C_null_char, &
          "tmp.couple"//C_null_char,c_loc(world),cs)
ELSE
  CALL error("Invalid mode")
ENDIF

! setup data
   
nper = 3
nlocal = nlen/nprocs
IF (me < MOD(nlen,nprocs)) nlocal = nlocal + 1

allocate(ids(nlocal))
m = 1
DO i = me+1,nlen,nprocs
  ids(m) = i
  m = m + 1
ENDDO

allocate(sint(nlocal))
allocate(sint64(nlocal))
allocate(sfloat(nlocal))
allocate(sdouble(nlocal))
ALLOCATE(sarray(nper,nlocal))

allocate(rint(nlocal))
allocate(rint64(nlocal))
allocate(rfloat(nlocal))
allocate(rdouble(nlocal))
ALLOCATE(rarray(nper,nlocal))

sstring = "world"

! endless server loop 

do

  ! receive message from client
  ! msgID = 0 = all-done message

  msgID = cslib_recv(cs,nfield,pfieldID,pfieldtype,pfieldlen)
  if (msgID == 0) exit
  CALL C_F_POINTER(pfieldID,fieldID,[nfield])
  CALL C_F_POINTER(pfieldtype,fieldtype,[nfield])
  CALL C_F_POINTER(pfieldlen,fieldlen,[nfield])

  CALL cslib_unpack_parallel(cs,1,nlocal,C_LOC(ids),1,c_loc(rint))
  CALL cslib_unpack_parallel(cs,2,nlocal,c_loc(ids),1,c_loc(rint64))
  CALL cslib_unpack_parallel(cs,3,nlocal,c_loc(ids),1,c_loc(rfloat))
  CALL cslib_unpack_parallel(cs,4,nlocal,c_loc(ids),1,c_loc(rdouble))
  CALL cslib_unpack_parallel(cs,5,nlocal,c_loc(ids),nper,c_loc(rarray))

  oneint = cslib_unpack_int(cs,6)
  oneint64 = cslib_unpack_int64(cs,7)
  onefloat = cslib_unpack_float(cs,8)
  onedouble = cslib_unpack_double(cs,9)
  ptr = cslib_unpack_string(cs,10)
  CALL C_F_POINTER(ptr,rstring,[fieldlen(10)-1])

  ! increment values

  do i = 1,nlocal
    sint(i) = rint(i) + 1
    sint64(i) = rint64(i) + 1
    sfloat(i) = rfloat(i) + 1.0
    sdouble(i) = rdouble(i) + 1.0
    do j = 1,nper
       sarray(j,i) = rarray(j,i) + 1.0
    enddo
  enddo

  ! send message to server

  call cslib_send(cs,1,10)
  CALL cslib_pack_parallel(cs,1,1,nlocal,c_loc(ids),1,c_loc(sint))
  CALL cslib_pack_parallel(cs,2,2,nlocal,c_loc(ids),1,c_loc(sint64))
  CALL cslib_pack_parallel(cs,3,3,nlocal,c_loc(ids),1,c_loc(sfloat))
  CALL cslib_pack_parallel(cs,4,4,nlocal,c_loc(ids),1,c_loc(sdouble))
  CALL cslib_pack_parallel(cs,5,4,nlocal,c_loc(ids),nper,c_loc(sarray))
  CALL cslib_pack_int(cs,6,oneint+1)
  CALL cslib_pack_int64(cs,7,oneint64+1)
  CALL cslib_pack_float(cs,8,onefloat+1)
  CALL cslib_pack_double(cs,9,onedouble+1)
  CALL cslib_pack_string(cs,10,TRIM(sstring)//c_null_char)

enddo

! final reply to client

call cslib_send(cs,0,0)

! clean-up

deallocate(ids)

deallocate(sint)
deallocate(sint64)
deallocate(sfloat)
deallocate(sdouble)
deallocate(sarray)
  
deallocate(rint)
deallocate(rint64)
deallocate(rfloat)
deallocate(rdouble)
deallocate(rarray)

call cslib_close(cs)
IF (mpifree == 1) CALL MPI_Comm_free(world,ierr)
call MPI_FINALIZE(ierr)

end program server_parallel

! utility functions

subroutine error(str)
character (len=*) :: str

print *,"ERROR: ",str
call exit()

end subroutine error
