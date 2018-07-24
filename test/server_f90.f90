! serial server example in Fortran

! main program

program server

USE ISO_C_binding
USE cslib_wrap

implicit none

INTEGER :: i,j
integer :: nlen,nper
integer :: argc
TYPE(C_ptr) :: cs,ptr,pfieldID,pfieldtype,pfieldlen
CHARACTER(len=32) :: mode,arg
INTEGER :: msgID,nfield

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

! command-line args

argc = command_argument_count()

IF (argc /= 2) THEN
  print *,"Syntax: server_f90 mode N"
  print *,"  mode = file or zmq"
  print *,"  N = length of data vectors"
  call exit(1)
endif

call get_command_argument(1,mode)
call get_command_argument(2,arg)
read (arg,"(i10)") nlen

! setup messaging

if (mode == "file") then
  CALL cslib_open_fortran(1,TRIM(mode)//C_null_char, &
          "tmp.couple"//C_null_char,c_null_ptr,cs)
else if (mode == "zmq") then
  CALL cslib_open_fortran(1,TRIM(mode)//C_null_char, &
          "*:5555"//C_null_char,c_null_ptr,cs)
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

  ! increment values

  do i = 1,nlen
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
  call cslib_pack(cs,1,1,nlen,c_loc(sint))
  call cslib_pack(cs,2,2,nlen,c_loc(sint64))
  call cslib_pack(cs,3,3,nlen,c_loc(sfloat))
  call cslib_pack(cs,4,4,nlen,c_loc(sdouble))
  call cslib_pack(cs,5,4,nper*nlen,c_loc(sarray))
  CALL cslib_pack_int(cs,6,oneint+1)
  CALL cslib_pack_int64(cs,7,oneint64+1)
  CALL cslib_pack_float(cs,8,onefloat+1)
  CALL cslib_pack_double(cs,9,onedouble+1)
  CALL cslib_pack_string(cs,10,TRIM(sstring)//c_null_char)

enddo

! final reply to client

call cslib_send(cs,0,0)

! clean-up

deallocate(sint)
deallocate(sint64)
deallocate(sfloat)
deallocate(sdouble)
deallocate(sarray)

call cslib_close(cs)

end program server

! utility functions

subroutine error(str)
character (len=*) :: str

print *,"ERROR: ",str
call exit()

end subroutine error
