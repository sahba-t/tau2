
c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine init_comm 

c---------------------------------------------------------------------
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c
c   initialize MPI and establish rank and size
c
c This is a module in the MPI implementation of LUSSOR
c pseudo application from the NAS Parallel Benchmarks. 
c
c---------------------------------------------------------------------

      implicit none

      include 'mpinpb.h'
      include 'applu.incl'

      integer profiler(2)
      save profiler
      integer nodedim
      integer IERROR


c---------------------------------------------------------------------
c    initialize MPI communication
c---------------------------------------------------------------------
      call TAU_PROFILE_TIMER(profiler, 'init_comm');
      call TAU_PROFILE_START(profiler);
      call MPI_INIT( IERROR )

c---------------------------------------------------------------------
c   establish the global rank of this process
c---------------------------------------------------------------------
      call MPI_COMM_RANK( MPI_COMM_WORLD,
     >                     id,
     >                     IERROR )
      call TAU_PROFILE_SET_NODE(id)

c---------------------------------------------------------------------
c   establish the size of the global group
c---------------------------------------------------------------------
      call MPI_COMM_SIZE( MPI_COMM_WORLD,
     >                     num,
     >                     IERROR )

      ndim   = nodedim(num)

      if (.not. convertdouble) then
         dp_type = MPI_DOUBLE_PRECISION
      else
         dp_type = MPI_REAL
      endif


      call TAU_PROFILE_STOP(profiler);
      return
      end
