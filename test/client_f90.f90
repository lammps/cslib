! serial client example in Fortran

! main program

program client

USE ISO_C_binding
USE cslib_wrap

implicit none

INTEGER :: i,j,iter
integer :: nlen,niter,nper
REAL(8) :: tstart,tstop,bandwidth,delta,one
INTEGER :: argc,datumsize
TYPE(C_ptr) :: cs,ptr,pfieldID,pfieldtype,pfieldlen
CHARACTER(len=32) :: mode,arg
INTEGER :: msgID,nfield,nsend,nrecv

INTEGER(C_INT), POINTER :: fieldID(:) => NULL()
INTEGER(C_INT), POINTER :: fieldtype(:) => NULL()
INTEGER(C_INT), POINTER :: fieldlen(:) => NULL()

INTEGER(4), ALLOCATABLE, target :: sint(:)
INTEGER(8), ALLOCATABLE, target :: sint64(:)
REAL(4), ALLOCATABLE, target :: sfloat(:)
REAL(8), ALLOCATABLE, target :: sdouble(:)
REAL(8), ALLOCATABLE, target :: sarray(:,:)

INTEGER(4), POINTER :: rint(:) => NULL()
INTEGER(8), POINTER :: rint64(:) => NULL()
REAL(4), POINTER :: rfloat(:) => NULL()
REAL(8), POINTER :: rdouble(:) => NULL()
REAL(8), POINTER :: rarray(:,:) => NULL()

!INTEGER(C_INT), POINTER :: rint(:) => NULL()
!INTEGER(C_int64_t), POINTER :: rint64(:) => NULL()
!REAL(C_float), POINTER :: rfloat(:) => NULL()
!REAL(C_double), POINTER :: rdouble(:) => NULL()
!REAL(C_double), POINTER :: rarray(:,:) => NULL()

integer(4) :: oneint
integer(8) :: oneint64
real(4) :: onefloat
real(8) :: onedouble
CHARACTER(len=32) :: sstring
CHARACTER(c_char), POINTER :: rstring(:) => NULL()

REAL(8), external :: timer

INTEGER(8) :: BIGOFFSET = 3000000
REAL(4) :: FLOATOFFSET = 0.5

! command-line args

argc = command_argument_count()

IF (argc /= 3) THEN
  print *,"Syntax: client_f90 mode N Niter"
  print *,"  mode = file or zmq"
  print *,"  N = length of data vectors"
  print *,"  Niter = # of iterations"
  call exit(1)
endif

call get_command_argument(1,mode)
call get_command_argument(2,arg)
read (arg,"(i10)") nlen
call get_command_argument(3,arg)
read (arg,"(i10)") niter

! setup messaging

if (mode == "file") then
  CALL cslib_open_fortran(0,TRIM(mode)//C_null_char, &
          "tmp.couple"//C_null_char,c_null_ptr,cs)
else if (mode == "zmq") then
  CALL cslib_open_fortran(0,TRIM(mode)//C_null_char, &
          "localhost:5555"//C_null_char,c_null_ptr,cs)
else 
  CALL error("Invalid mode")
endif

! setup data
   
nper = 3

allocate(sint(nlen))
allocate(sint64(nlen))
allocate(sfloat(nlen))
allocate(sdouble(nlen))
ALLOCATE(sarray(nper,nlen))

do i = 1,nlen
  sint(i) = i
  sint64(i) = BIGOFFSET + i
  sfloat(i) = FLOATOFFSET + i
  sdouble(i) = BIGOFFSET + FLOATOFFSET + i
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
 
  CALL cslib_send(cs,1,10)
  call cslib_pack(cs,1,1,nlen,c_loc(sint))
  call cslib_pack(cs,2,2,nlen,c_loc(sint64))
  call cslib_pack(cs,3,3,nlen,c_loc(sfloat))
  call cslib_pack(cs,4,4,nlen,c_loc(sdouble))
  call cslib_pack(cs,5,4,nper*nlen,c_loc(sarray))
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
  
  ptr = cslib_unpack(cs,1)
  CALL C_F_POINTER(ptr,rint,[nlen])
  ptr = cslib_unpack(cs,2)
  CALL C_F_POINTER(ptr,rint64,[nlen])
  ptr = cslib_unpack(cs,3)
  CALL C_F_POINTER(ptr,rfloat,[nlen])
  ptr = cslib_unpack(cs,4)
  CALL C_F_POINTER(ptr,rdouble,[nlen])
  ptr = cslib_unpack(cs,5)
  CALL C_F_POINTER(ptr,rarray,[nper,nlen])

  oneint = cslib_unpack_int(cs,6)
  oneint64 = cslib_unpack_int64(cs,7)
  onefloat = cslib_unpack_float(cs,8)
  onedouble = cslib_unpack_double(cs,9)
  ptr = cslib_unpack_string(cs,10)
  CALL C_F_POINTER(ptr,rstring,[fieldlen(10)-1])

  ! copy server data to client data

  do i = 1,nlen
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

do i = 1,nlen
  one = i + niter - sint(i)
  delta = MAX(delta,abs(one))
  one = BIGOFFSET + i + niter - sint64(i)
  delta = MAX(delta,abs(one))
  one = FLOATOFFSET + i + niter - sfloat(i)
  delta = MAX(delta,abs(one))
  one = BIGOFFSET+FLOATOFFSET + i + niter - sdouble(i)
  delta = MAX(delta,abs(one))
  do j = 1,nper
    one = BIGOFFSET+FLOATOFFSET + i + niter - sarray(j,i)
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

! stats

nsend = cslib_extract(cs,1)
nrecv = cslib_extract(cs,2)

PRINT *,"Nsend,nrecv:",nsend,nrecv
print *,"Initial/final first int:",1,sint(1)
print *,"Initial/final last int:",nlen,sint(nlen)
print *,"Initial/final first int64:",BIGOFFSET+1,sint64(1)
print *,"Initial/final last int64:",BIGOFFSET+nlen,sint64(nlen)
print *,"Initial/final first float:",FLOATOFFSET+1,sfloat(1)
print *,"Initial/final last float:",FLOATOFFSET+nlen,sfloat(nlen)
print *,"Initial/final first double:",BIGOFFSET+FLOATOFFSET+1,sdouble(1)
print *,"Initial/final last double:",BIGOFFSET+FLOATOFFSET+nlen,sdouble(nlen)
print *,"Initial/final first array:",BIGOFFSET+FLOATOFFSET+1,sarray(1,1)
PRINT *,"Initial/final last array:",BIGOFFSET+FLOATOFFSET+nlen,sarray(3,nlen)
PRINT *,"Initial/final int scalar:",1,oneint
PRINT *,"Initial/final int64 scalar:",BIGOFFSET+1,oneint64
PRINT *,"Initial/final float scalar:",FLOATOFFSET+1,onefloat
PRINT *,"Initial/final double scalar:",BIGOFFSET+FLOATOFFSET+1,onedouble
PRINT *,"Initial/final string: ",trim(sstring)," ",rstring

print *,"Max error for any value:",delta
print *,"CPU time:",tstop-tstart
datumsize = 4 + 8 + 4 + 8 + nper*8
bandwidth = 2.0 * nlen * datumsize / (tstop-tstart) / 1024/1024
print *,"Message bandwidth:",bandwidth,"MB/sec"

! clean-up

deallocate(sint)
deallocate(sint64)
deallocate(sfloat)
deallocate(sdouble)
deallocate(sarray)

call cslib_close(cs)

end program client

! utility functions

subroutine error(str)
character (len=*) :: str

print *,"ERROR:",str
call exit()

end subroutine error
