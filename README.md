A small planner/reminder tui application

Wanted to do this for a long time, but also got inspiration from dear Mister Azozin (https://github.com/rexim/tore)

# How to use:

o - add a new item on the same level

a - add a new item as a child

Enter - submit new item's name

j - move down

k - move up

c - change item's name

Enter - mark item as done

Tab - collapse an item

d - delete selection (deleted tasks get copied)

v - enable multiple line selection

Esc - exit multiple selection mode

p - paste after selected task

P - paste as a child of selected task

q - exit

Q - exit without saving

w - save

# Known issues:
- Can't exit visual and insert mode using Escape key on Windows

# TODO:
- [x] General Windows support (shouldn't be too hard [clueless])
- [x] Configuration/customization
- [ ] Implement moving tasks up and down (J and K)
- [ ] Multiple "workspaces"
- [ ] Make config paths work on Windows
- [ ] Recurring tasks
- [ ] Automatic reminders
