! parallel client example in Fortran

! main program

program client_parallel

USE ISO_C_binding
USE cslib_wrap

implicit none
include 'mpif.h'

INTEGER :: i,j,m,iter,ierr
INTEGER :: onlyclient,me,nprocs,mpifree
INTEGER :: nlen,nlocal,niter,nper
REAL(8) :: tstart,tstop,bandwidth,delta,alldelta,one
INTEGER :: argc,datumsize
TYPE(C_ptr) :: cs,ptr,pfieldID,pfieldtype,pfieldlen
CHARACTER(len=32) :: mode,arg
INTEGER :: msgID,nfield,nsend,nrecv

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

REAL(8), external :: timer

INTEGER(8) :: BIGOFFSET = 3000000    ! needs to be 1000x larger
REAL(4) :: FLOATOFFSET = 0.5

! MPI setup

call MPI_INIT(ierr)
world = MPI_COMM_WORLD
call MPI_COMM_RANK(world,me,ierr)
call MPI_COMM_SIZE(world,nprocs,ierr)

! command-line args

argc = command_argument_count()

IF (argc /= 3) THEN
  IF (me == 0) THEN
    PRINT *,"Syntax: mpirun -np P client_parallel_f90 mode N Niter"
    PRINT *,"  mode = file or zmq or mpi/one or mpi/two"
    PRINT *,"  N = length of data vectors"
    PRINT *,"  Niter = # of iterations"
  ENDIF
  CALL EXIT(1)
endif

CALL GET_COMMAND_ARGUMENT(1,mode)
CALL GET_COMMAND_ARGUMENT(2,arg) 
READ (arg,"(i10)") nlen
CALL GET_COMMAND_ARGUMENT(3,arg)
READ (arg,"(i10)") niter

! setup messaging

mpifree = 0

IF (mode == "file") THEN
  CALL cslib_open_fortran(0,TRIM(mode)//C_null_char, &
          "tmp.couple"//C_null_char,c_loc(world),cs)
else if (mode == "zmq") then
  CALL cslib_open_fortran(0,TRIM(mode)//C_null_char, &
          "localhost:5555"//C_null_char,c_loc(world),cs)
ELSE IF (mode == "mpi/one") THEN
  mpifree = 1
  CALL MPI_Comm_split(world,0,me,onlyclient,ierr)
  CALL MPI_Comm_rank(onlyclient,me,ierr)
  CALL MPI_Comm_size(onlyclient,nprocs,ierr)
  both = world
  world = onlyclient
  CALL cslib_open_fortran_mpi_one(0,TRIM(mode)//C_null_char, &
          C_LOC(both),C_LOC(world),cs)
ELSE IF (mode == "mpi/two") THEN
  CALL cslib_open_fortran(0,TRIM(mode)//C_null_char, &
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

do i = 1,nlocal
  sint(i) = ids(i)
  sint64(i) = BIGOFFSET + ids(i)
  sfloat(i) = FLOATOFFSET + ids(i)
  sdouble(i) = BIGOFFSET + FLOATOFFSET + ids(i)
  do j = 1,nper
    sarray(j,i) = sdouble(i)
  enddo
enddo

oneint = 1
oneint64 = BIGOFFSET + 1
onefloat = FLOATOFFSET + 1
onedouble = BIGOFFSET + FLOATOFFSET + 1
 
sstring = "hello"

! client loop
   
call cpu_time(tstart)

do iter = 1,niter

  ! send message to server

  call cslib_send(cs,1,10)
  CALL cslib_pack_parallel(cs,1,1,nlocal,c_loc(ids),1,c_loc(sint))
  CALL cslib_pack_parallel(cs,2,2,nlocal,c_loc(ids),1,c_loc(sint64))
  CALL cslib_pack_parallel(cs,3,3,nlocal,c_loc(ids),1,c_loc(sfloat))
  CALL cslib_pack_parallel(cs,4,4,nlocal,c_loc(ids),1,c_loc(sdouble))
  CALL cslib_pack_parallel(cs,5,4,nlocal,c_loc(ids),nper,c_loc(sarray))
  CALL cslib_pack_int(cs,6,oneint)
  CALL cslib_pack_int64(cs,7,oneint64)
  CALL cslib_pack_float(cs,8,onefloat)
  CALL cslib_pack_double(cs,9,onedouble)
  CALL cslib_pack_string(cs,10,trim(sstring)//c_null_char)

  ! receive message from server

  msgID = cslib_recv(cs,nfield,pfieldID,pfieldtype,pfieldlen)
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

  ! copy server data to client data

  do i = 1,nlocal
    sint(i) = rint(i)
    sint64(i) = rint64(i)
    sfloat(i) = rfloat(i)
    sdouble(i) = rdouble(i)
    do j = 1,nper
      sarray(j,i) = rarray(j,i)
    enddo
  enddo

enddo

call cpu_time(tstop)

! all-done message to server

call cslib_send(cs,0,0)
msgID = cslib_recv(cs,nfield,pfieldID,pfieldtype,pfieldlen)

! error check

delta = 0.0

do i = 1,nlocal
  one = ids(i) + niter - sint(i)
  delta = MAX(delta,abs(one))
  one = BIGOFFSET + ids(i) + niter - sint64(i)
  delta = MAX(delta,abs(one))
  one = FLOATOFFSET + ids(i) + niter - sfloat(i)
  delta = MAX(delta,abs(one))
  one = BIGOFFSET+FLOATOFFSET + ids(i) + niter - sdouble(i)
  delta = MAX(delta,abs(one))
  do j = 1,nper
    one = BIGOFFSET+FLOATOFFSET + ids(i) + niter - sarray(j,i)
    delta = MAX(delta,abs(one))
  enddo
enddo 

one = 1 + niter - oneint
delta = MAX(delta,abs(one))
one = BIGOFFSET+1 + niter - oneint64
delta = MAX(delta,abs(one))
one = FLOATOFFSET+1 + niter - onefloat
delta = MAX(delta,abs(one))
one = BIGOFFSET+FLOATOFFSET+1 + niter - onedouble
delta = MAX(delta,abs(one))

call MPI_Allreduce(delta,alldelta,1,MPI_DOUBLE,MPI_MAX,world)
delta = alldelta

! stats

nsend = cslib_extract(cs,1)
nrecv = cslib_extract(cs,2)

if (me == 0) then
  PRINT *,"Nsend,nrecv:",nsend,nrecv
  PRINT *,"Initial/final first int:",1,sint(1)
  PRINT *,"Initial/final last int:",nlen,sint(nlocal)
  PRINT *,"Initial/final first int64:",BIGOFFSET+1,sint64(1)
  PRINT *,"Initial/final last int64:",BIGOFFSET+nlen,sint64(nlocal)
  PRINT *,"Initial/final first float:",FLOATOFFSET+1,sfloat(1)
  PRINT *,"Initial/final last float:",FLOATOFFSET+nlen,sfloat(nlocal)
  PRINT *,"Initial/final first double:",BIGOFFSET+FLOATOFFSET+1,sdouble(1)
  PRINT *,"Initial/final last double:",BIGOFFSET+FLOATOFFSET+nlen,sdouble(nlocal)
  PRINT *,"Initial/final first array:",BIGOFFSET+FLOATOFFSET+1,sarray(1,1)
  PRINT *,"Initial/final last array:",BIGOFFSET+FLOATOFFSET+nlen, &
          sarray(nper,nlocal)
  PRINT *,"Initial/final int scalar:",1,oneint
  PRINT *,"Initial/final int64 scalar:",BIGOFFSET+1,oneint64
  PRINT *,"Initial/final float scalar:",FLOATOFFSET+1,onefloat
  PRINT *,"Initial/final double scalar:",BIGOFFSET+FLOATOFFSET+1,onedouble
  PRINT *,"Initial/final string: ",TRIM(sstring)," ",rstring
  
  PRINT *,"Max error for any value:",delta
  PRINT *,"CPU time:",tstop-tstart
  datumsize = 4 + 8 + 4 + 8 + nper*8
  bandwidth = 2.0 * nlen * datumsize / (tstop-tstart) / 1024/1024
  PRINT *,"Message bandwidth:",bandwidth,"MB/sec"
endif

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

end program client_parallel

! utility functions

subroutine error(str)
character (len=*) :: str

print *,"ERROR:",str
call exit()

end subroutine error
